/*
 *  This file is part of nzbget
 *
 *  Copyright (C) 2005 Bo Cordes Petersen <placebodk@sourceforge.net>
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
 * $Revision: 1149 $
 * $Date: 2014-10-21 21:32:07 +0200 (mar. 21 oct. 2014) $
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
#ifndef WIN32
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#endif

#include "nzbget.h"
#include "RemoteServer.h"
#include "BinRpc.h"
#include "WebServer.h"
#include "Log.h"
#include "Options.h"
#include "Util.h"

extern Options* g_pOptions;

//*****************************************************************
// RemoteServer

RemoteServer::RemoteServer(bool bTLS)
{
	debug("Creating RemoteServer");

	m_bTLS = bTLS;
	m_pConnection = NULL;
}

RemoteServer::~RemoteServer()
{
	debug("Destroying RemoteServer");

	delete m_pConnection;
}

void RemoteServer::Run()
{
	debug("Entering RemoteServer-loop");

#ifndef DISABLE_TLS
	if (m_bTLS)
	{
		if (strlen(g_pOptions->GetSecureCert()) == 0 || !Util::FileExists(g_pOptions->GetSecureCert()))
		{
			error("Could not initialize TLS, secure certificate is not configured or the cert-file was not found. Check option <SecureCert>");
			return;
		}

		if (strlen(g_pOptions->GetSecureKey()) == 0 || !Util::FileExists(g_pOptions->GetSecureKey()))
		{
			error("Could not initialize TLS, secure key is not configured or the key-file was not found. Check option <SecureKey>");
			return;
		}
	}
#endif

	while (!IsStopped())
	{
		bool bBind = true;

		if (!m_pConnection)
		{
			m_pConnection = new Connection(g_pOptions->GetControlIP(), 
				m_bTLS ? g_pOptions->GetSecurePort() : g_pOptions->GetControlPort(),
				m_bTLS);
			m_pConnection->SetTimeout(g_pOptions->GetUrlTimeout());
			m_pConnection->SetSuppressErrors(false);
			bBind = m_pConnection->Bind();
		}

		// Accept connections and store the new Connection
		Connection* pAcceptedConnection = NULL;
		if (bBind)
		{
			pAcceptedConnection = m_pConnection->Accept();
		}
		if (!bBind || pAcceptedConnection == NULL)
		{
			// Remote server could not bind or accept connection, waiting 1/2 sec and try again
			if (IsStopped())
			{
				break; 
			}
			usleep(500 * 1000);
			delete m_pConnection;
			m_pConnection = NULL;
			continue;
		}

		RequestProcessor* commandThread = new RequestProcessor();
		commandThread->SetAutoDestroy(true);
		commandThread->SetConnection(pAcceptedConnection);
#ifndef DISABLE_TLS
		commandThread->SetTLS(m_bTLS);
#endif
		commandThread->Start();
	}

	if (m_pConnection)
	{
		m_pConnection->Disconnect();
	}

	debug("Exiting RemoteServer-loop");
}

void RemoteServer::Stop()
{
	Thread::Stop();
	if (m_pConnection)
	{
		m_pConnection->SetSuppressErrors(true);
		m_pConnection->Cancel();
#ifdef WIN32
		m_pConnection->Disconnect();
#endif
	}
}

//*****************************************************************
// RequestProcessor

RequestProcessor::~RequestProcessor()
{
	m_pConnection->Disconnect();
	delete m_pConnection;
}

void RequestProcessor::Run()
{
	bool bOK = false;

	m_pConnection->SetSuppressErrors(true);

#ifndef DISABLE_TLS
	if (m_bTLS && !m_pConnection->StartTLS(false, g_pOptions->GetSecureCert(), g_pOptions->GetSecureKey()))
	{
		debug("Could not establish secure connection to web-client: Start TLS failed");
		return;
	}
#endif

	// Read the first 4 bytes to determine request type
	int iSignature = 0;
	if (!m_pConnection->Recv((char*)&iSignature, 4))
	{
		debug("Could not read request signature");
		return;
	}

	if ((int)ntohl(iSignature) == (int)NZBMESSAGE_SIGNATURE)
	{
		// binary request received
		bOK = true;
		BinRpcProcessor processor;
		processor.SetConnection(m_pConnection);
		processor.Execute();
	}
	else if (!strncmp((char*)&iSignature, "POST", 4) || 
		!strncmp((char*)&iSignature, "GET ", 4) ||
		!strncmp((char*)&iSignature, "OPTI", 4))
	{
		// HTTP request received
		char szBuffer[1024];
		if (m_pConnection->ReadLine(szBuffer, sizeof(szBuffer), NULL))
		{
			WebProcessor::EHttpMethod eHttpMethod = WebProcessor::hmGet;
			char* szUrl = szBuffer;
			if (!strncmp((char*)&iSignature, "POST", 4))
			{
				eHttpMethod = WebProcessor::hmPost;
				szUrl++;
			}
			if (!strncmp((char*)&iSignature, "OPTI", 4) && strlen(szUrl) > 4)
			{
				eHttpMethod = WebProcessor::hmOptions;
				szUrl += 4;
			}
			if (char* p = strchr(szUrl, ' '))
			{
				*p = '\0';
			}

			debug("url: %s", szUrl);

			WebProcessor processor;
			processor.SetConnection(m_pConnection);
			processor.SetUrl(szUrl);
			processor.SetHttpMethod(eHttpMethod);
			processor.Execute();
			bOK = true;
		}
	}

	if (!bOK)
	{
		warn("Non-nzbget request received on port %i from %s", m_bTLS ? g_pOptions->GetSecurePort() : g_pOptions->GetControlPort(), m_pConnection->GetRemoteAddr());
	}
}
