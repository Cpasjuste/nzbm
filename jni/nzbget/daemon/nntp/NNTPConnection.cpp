/*
 *  This file is part of nzbget
 *
 *  Copyright (C) 2004 Sven Henkel <sidddy@users.sourceforge.net>
 *  Copyright (C) 2007-2013 Andrey Prygunkov <hugbug@users.sourceforge.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * $Revision: 956 $
 * $Date: 2014-02-24 23:11:14 +0100 (Mon, 24 Feb 2014) $
 *
 */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef WIN32
#include "win32.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#include "nzbget.h"
#include "Log.h"
#include "NNTPConnection.h"
#include "Connection.h"
#include "NewsServer.h"

static const int CONNECTION_LINEBUFFER_SIZE = 1024*10;

NNTPConnection::NNTPConnection(NewsServer* pNewsServer) : Connection(pNewsServer->GetHost(), pNewsServer->GetPort(), pNewsServer->GetTLS())
{
	m_pNewsServer = pNewsServer;
	m_szActiveGroup = NULL;
	m_szLineBuf = (char*)malloc(CONNECTION_LINEBUFFER_SIZE);
	m_bAuthError = false;
	SetCipher(pNewsServer->GetCipher());
}

NNTPConnection::~NNTPConnection()
{
	free(m_szActiveGroup);
	free(m_szLineBuf);
}

const char* NNTPConnection::Request(const char* req)
{
	if (!req)
	{
		return NULL;
	}

	m_bAuthError = false;

	WriteLine(req);

	char* answer = ReadLine(m_szLineBuf, CONNECTION_LINEBUFFER_SIZE, NULL);

	if (!answer)
	{
		return NULL;
	}

	if (!strncmp(answer, "480", 3))
	{
		debug("%s requested authorization", GetHost());

		if (!Authenticate())
		{
			return NULL;
		}

		//try again
		WriteLine(req);
		answer = ReadLine(m_szLineBuf, CONNECTION_LINEBUFFER_SIZE, NULL);
	}

	return answer;
}

bool NNTPConnection::Authenticate()
{
	if (strlen(m_pNewsServer->GetUser()) == 0 || strlen(m_pNewsServer->GetPassword()) == 0)
	{
		error("%c%s (%s) requested authorization but username/password are not set in settings", 
			toupper(m_pNewsServer->GetName()[0]), m_pNewsServer->GetName() + 1, m_pNewsServer->GetHost());
		m_bAuthError = true;
		return false;
	}

	m_bAuthError = !AuthInfoUser(0);
	return !m_bAuthError;
}

bool NNTPConnection::AuthInfoUser(int iRecur)
{
	if (iRecur > 10)
	{
		return false;
	}

	char tmp[1024];
	snprintf(tmp, 1024, "AUTHINFO USER %s\r\n", m_pNewsServer->GetUser());
	tmp[1024-1] = '\0';

	WriteLine(tmp);

	char* answer = ReadLine(m_szLineBuf, CONNECTION_LINEBUFFER_SIZE, NULL);
	if (!answer)
	{
		ReportErrorAnswer("Authorization for server%i (%s) failed: Connection closed by remote host", NULL);
		return false;
	}

	if (!strncmp(answer, "281", 3))
	{
		debug("Authorization for %s successful", GetHost());
		return true;
	}
	else if (!strncmp(answer, "381", 3))
	{
		return AuthInfoPass(++iRecur);
	}
	else if (!strncmp(answer, "480", 3))
	{
		return AuthInfoUser(++iRecur);
	}

	if (char* p = strrchr(answer, '\r')) *p = '\0'; // remove last CRLF from error message

	if (GetStatus() != csCancelled)
	{
		ReportErrorAnswer("Authorization for server%i (%s) failed (Answer: %s)", answer);
	}
	return false;
}

bool NNTPConnection::AuthInfoPass(int iRecur)
{
	if (iRecur > 10)
	{
		return false;
	}

	char tmp[1024];
	snprintf(tmp, 1024, "AUTHINFO PASS %s\r\n", m_pNewsServer->GetPassword());
	tmp[1024-1] = '\0';

	WriteLine(tmp);

	char* answer = ReadLine(m_szLineBuf, CONNECTION_LINEBUFFER_SIZE, NULL);
	if (!answer)
	{
		ReportErrorAnswer("Authorization for server%i (%s) failed: Connection closed by remote host", NULL);
		return false;
	}
	else if (!strncmp(answer, "2", 1))
	{
		debug("Authorization for %s successful", GetHost());
		return true;
	}
	else if (!strncmp(answer, "381", 3))
	{
		return AuthInfoPass(++iRecur);
	}

	if (char* p = strrchr(answer, '\r')) *p = '\0'; // remove last CRLF from error message

	if (GetStatus() != csCancelled)
	{
		ReportErrorAnswer("Authorization for server%i (%s) failed (Answer: %s)", answer);
	}
	return false;
}

const char* NNTPConnection::JoinGroup(const char* grp)
{
	if (m_szActiveGroup && !strcmp(m_szActiveGroup, grp))
	{
		// already in group
		strcpy(m_szLineBuf, "211 ");
		return m_szLineBuf;
	}

	char tmp[1024];
	snprintf(tmp, 1024, "GROUP %s\r\n", grp);
	tmp[1024-1] = '\0';

	const char* answer = Request(tmp);

	if (answer && !strncmp(answer, "2", 1))
	{
		debug("Changed group to %s on %s", grp, GetHost());
		free(m_szActiveGroup);
		m_szActiveGroup = strdup(grp);
	}
	else
	{
		debug("Error changing group on %s to %s: %s.", GetHost(), grp, answer);
	}

	return answer;
}

bool NNTPConnection::Connect()
{
	debug("Opening connection to %s", GetHost());

	if (m_eStatus == csConnected)
	{
		return true;
	}

	if (!Connection::Connect())
	{
		return false;
	}

	char* answer = ReadLine(m_szLineBuf, CONNECTION_LINEBUFFER_SIZE, NULL);

	if (!answer)
	{
		ReportErrorAnswer("Connection to server%i (%s) failed: Connection closed by remote host", NULL);
		Disconnect();
		return false;
	}

	if (strncmp(answer, "2", 1))
	{
		ReportErrorAnswer("Connection to server%i (%s) failed (Answer: %s)", answer);
		Disconnect();
		return false;
	}

	if ((strlen(m_pNewsServer->GetUser()) > 0 && strlen(m_pNewsServer->GetPassword()) > 0) &&
		!Authenticate())
	{
		return false;
	}

	debug("Connection to %s established", GetHost());

	return true;
}

bool NNTPConnection::Disconnect()
{
	if (m_eStatus == csConnected)
	{
		Request("quit\r\n");
		free(m_szActiveGroup);
		m_szActiveGroup = NULL;
	}
	return Connection::Disconnect();
}

void NNTPConnection::ReportErrorAnswer(const char* szMsgPrefix, const char* szAnswer)
{
	char szErrStr[1024];
	snprintf(szErrStr, 1024, szMsgPrefix, m_pNewsServer->GetID(), m_pNewsServer->GetHost(), szAnswer);
	szErrStr[1024-1] = '\0';
	
	ReportError(szErrStr, NULL, false, 0);
}
