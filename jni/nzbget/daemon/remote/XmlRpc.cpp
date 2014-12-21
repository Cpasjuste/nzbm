/*
 *  This file is part of nzbget
 *
 *  Copyright (C) 2007-2014 Andrey Prygunkov <hugbug@users.sourceforge.net>
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
 * $Revision: 1142 $
 * $Date: 2014-10-12 16:23:54 +0200 (Sun, 12 Oct 2014) $
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
#include <stdarg.h>
#ifndef WIN32
#include <unistd.h>
#endif

#include "nzbget.h"
#include "XmlRpc.h"
#include "Log.h"
#include "Options.h"
#include "Scanner.h"
#include "FeedCoordinator.h"
#include "ServerPool.h"
#include "Util.h"
#include "Maintenance.h"
#include "StatMeter.h"
#include "ArticleWriter.h"

extern Options* g_pOptions;
extern Scanner* g_pScanner;
extern FeedCoordinator* g_pFeedCoordinator;
extern ServerPool* g_pServerPool;
extern Maintenance* g_pMaintenance;
extern StatMeter* g_pStatMeter;
extern ArticleCache* g_pArticleCache;
extern void ExitProc();
extern void Reload();

class ErrorXmlCommand: public XmlCommand
{
private:
	int					m_iErrCode;
	const char*			m_szErrText;

public:
						ErrorXmlCommand(int iErrCode, const char* szErrText);
	virtual void		Execute();
};

class PauseUnpauseXmlCommand: public XmlCommand
{
public:
	enum EPauseAction
	{
		paDownload,
		paPostProcess,
		paScan
	};

private:
	bool			m_bPause;
	EPauseAction	m_eEPauseAction;

public:
						PauseUnpauseXmlCommand(bool bPause, EPauseAction eEPauseAction);
	virtual void		Execute();
};

class ScheduleResumeXmlCommand: public XmlCommand
{
public:
	virtual void		Execute();
};

class ShutdownXmlCommand: public XmlCommand
{
public:
	virtual void		Execute();
};

class ReloadXmlCommand: public XmlCommand
{
public:
	virtual void		Execute();
};

class VersionXmlCommand: public XmlCommand
{
public:
	virtual void		Execute();
};

class DumpDebugXmlCommand: public XmlCommand
{
public:
	virtual void		Execute();
};

class SetDownloadRateXmlCommand: public XmlCommand
{
public:
	virtual void		Execute();
};

class StatusXmlCommand: public XmlCommand
{
public:
	virtual void		Execute();
};

class LogXmlCommand: public XmlCommand
{
protected:
	virtual Log::Messages*	LockMessages();
	virtual void			UnlockMessages();
public:
	virtual void		Execute();
};

class NzbInfoXmlCommand: public XmlCommand
{
protected:
	void				AppendNZBInfoFields(NZBInfo* pNZBInfo);
	void				AppendPostInfoFields(PostInfo* pPostInfo, int iLogEntries, bool bPostQueue);
};

class ListFilesXmlCommand: public XmlCommand
{
public:
	virtual void		Execute();
};

class ListGroupsXmlCommand: public NzbInfoXmlCommand
{
private:
	const char*			DetectStatus(NZBInfo* pNZBInfo);
public:
	virtual void		Execute();
};

class EditQueueXmlCommand: public XmlCommand
{
public:
	virtual void		Execute();
};

class DownloadXmlCommand: public XmlCommand
{
public:
	virtual void		Execute();
};

class PostQueueXmlCommand: public NzbInfoXmlCommand
{
public:
	virtual void		Execute();
};

class WriteLogXmlCommand: public XmlCommand
{
public:
	virtual void		Execute();
};

class ClearLogXmlCommand: public XmlCommand
{
public:
	virtual void		Execute();
};

class ScanXmlCommand: public XmlCommand
{
public:
	virtual void		Execute();
};

class HistoryXmlCommand: public NzbInfoXmlCommand
{
private:
	const char*			DetectStatus(HistoryInfo* pHistoryInfo);
public:
	virtual void		Execute();
};

class UrlQueueXmlCommand: public XmlCommand
{
public:
	virtual void		Execute();
};

class ConfigXmlCommand: public XmlCommand
{
public:
	virtual void		Execute();
};

class LoadConfigXmlCommand: public XmlCommand
{
public:
	virtual void		Execute();
};

class SaveConfigXmlCommand: public XmlCommand
{
public:
	virtual void		Execute();
};

class ConfigTemplatesXmlCommand: public XmlCommand
{
public:
	virtual void		Execute();
};

class ViewFeedXmlCommand: public XmlCommand
{
private:
	bool				m_bPreview;

public:
						ViewFeedXmlCommand(bool bPreview);
	virtual void		Execute();
};

class FetchFeedXmlCommand: public XmlCommand
{
public:
	virtual void		Execute();
};

class EditServerXmlCommand: public XmlCommand
{
public:
	virtual void		Execute();
};

class ReadUrlXmlCommand: public XmlCommand
{
public:
	virtual void		Execute();
};

class CheckUpdatesXmlCommand: public XmlCommand
{
public:
	virtual void		Execute();
};

class StartUpdateXmlCommand: public XmlCommand
{
public:
	virtual void		Execute();
};

class LogUpdateXmlCommand: public LogXmlCommand
{
protected:
	virtual Log::Messages*	LockMessages();
	virtual void			UnlockMessages();
};

class ServerVolumesXmlCommand: public XmlCommand
{
public:
	virtual void		Execute();
};

class ResetServerVolumeXmlCommand: public XmlCommand
{
public:
	virtual void		Execute();
};


//*****************************************************************
// XmlRpcProcessor

XmlRpcProcessor::XmlRpcProcessor()
{
	m_szRequest = NULL;
	m_eProtocol = rpUndefined;
	m_eHttpMethod = hmPost;
	m_szUrl = NULL;
	m_szContentType = NULL;
}

XmlRpcProcessor::~XmlRpcProcessor()
{
	free(m_szUrl);
}

void XmlRpcProcessor::SetUrl(const char* szUrl)
{
	m_szUrl = strdup(szUrl);
}


bool XmlRpcProcessor::IsRpcRequest(const char* szUrl)
{
	return !strcmp(szUrl, "/xmlrpc") || !strncmp(szUrl, "/xmlrpc/", 8) ||
		!strcmp(szUrl, "/jsonrpc") || !strncmp(szUrl, "/jsonrpc/", 9) ||
		!strcmp(szUrl, "/jsonprpc") || !strncmp(szUrl, "/jsonprpc/", 10);
}

void XmlRpcProcessor::Execute()
{
	m_eProtocol = rpUndefined;
	if (!strcmp(m_szUrl, "/xmlrpc") || !strncmp(m_szUrl, "/xmlrpc/", 8))
	{
		m_eProtocol = XmlRpcProcessor::rpXmlRpc;
	}
	else if (!strcmp(m_szUrl, "/jsonrpc") || !strncmp(m_szUrl, "/jsonrpc/", 9))
	{
		m_eProtocol = rpJsonRpc;
	}
	else if (!strcmp(m_szUrl, "/jsonprpc") || !strncmp(m_szUrl, "/jsonprpc/", 10))
	{
		m_eProtocol = rpJsonPRpc;
	}
	else
	{
		error("internal error: invalid rpc-request: %s", m_szUrl);
		return;
	}

	Dispatch();
}

void XmlRpcProcessor::Dispatch()
{
	char* szRequest = m_szRequest;
	char szMethodName[100];
	szMethodName[0] = '\0';

	if (m_eHttpMethod == hmGet)
	{
		szRequest = m_szUrl + 1;
		char* pstart = strchr(szRequest, '/');
		if (pstart)
		{
			char* pend = strchr(pstart + 1, '?');
			if (pend) 
			{
				int iLen = (int)(pend - pstart - 1 < (int)sizeof(szMethodName) - 1 ? pend - pstart - 1 : (int)sizeof(szMethodName) - 1);
				strncpy(szMethodName, pstart + 1, iLen);
				szMethodName[iLen] = '\0';
				szRequest = pend + 1;
			}
			else
			{
				strncpy(szMethodName, pstart + 1, sizeof(szMethodName));
				szMethodName[sizeof(szMethodName) - 1] = '\0';
				szRequest = szRequest + strlen(szRequest);
			}
		}
	}
	else if (m_eProtocol == rpXmlRpc)
	{
		WebUtil::XmlParseTagValue(m_szRequest, "methodName", szMethodName, sizeof(szMethodName), NULL);
	} 
	else if (m_eProtocol == rpJsonRpc) 
	{
		int iValueLen = 0;
		if (const char* szMethodPtr = WebUtil::JsonFindField(m_szRequest, "method", &iValueLen))
		{
			strncpy(szMethodName, szMethodPtr + 1, iValueLen - 2);
			szMethodName[iValueLen - 2] = '\0';
		}
	}

	debug("MethodName=%s", szMethodName);

	if (!strcasecmp(szMethodName, "system.multicall") && m_eProtocol == rpXmlRpc && m_eHttpMethod == hmPost)
	{
		MutliCall();
	}
	else
	{
		XmlCommand* command = CreateCommand(szMethodName);
		command->SetRequest(szRequest);
		command->SetProtocol(m_eProtocol);
		command->SetHttpMethod(m_eHttpMethod);
		command->PrepareParams();
		command->Execute();
		BuildResponse(command->GetResponse(), command->GetCallbackFunc(), command->GetFault());
		delete command;
	}
}

void XmlRpcProcessor::MutliCall()
{
	bool bError = false;
	StringBuilder cStringBuilder;

	cStringBuilder.Append("<array><data>");

	char* szRequestPtr = m_szRequest;
	char* szCallEnd = strstr(szRequestPtr, "</struct>");
	while (szCallEnd)
	{
		*szCallEnd = '\0';
		debug("MutliCall, request=%s", szRequestPtr);
		char* szNameEnd = strstr(szRequestPtr, "</name>");
		if (!szNameEnd)
		{
			bError = true;
			break;
		}

		char szMethodName[100];
		szMethodName[0] = '\0';
		WebUtil::XmlParseTagValue(szNameEnd, "string", szMethodName, sizeof(szMethodName), NULL);
		debug("MutliCall, MethodName=%s", szMethodName);

		XmlCommand* command = CreateCommand(szMethodName);
		command->SetRequest(szRequestPtr);
		command->Execute();

		debug("MutliCall, Response=%s", command->GetResponse());

		bool bFault = !strncmp(command->GetResponse(), "<fault>", 7);
		bool bArray = !bFault && !strncmp(command->GetResponse(), "<array>", 7);
		if (!bFault && !bArray)
		{
			cStringBuilder.Append("<array><data>");
		}
		cStringBuilder.Append("<value>");
		cStringBuilder.Append(command->GetResponse());
		cStringBuilder.Append("</value>");
		if (!bFault && !bArray)
		{
			cStringBuilder.Append("</data></array>");
		}

		delete command;

		szRequestPtr = szCallEnd + 9; //strlen("</struct>")
		szCallEnd = strstr(szRequestPtr, "</struct>");
	}

	if (bError)
	{
		XmlCommand* command = new ErrorXmlCommand(4, "Parse error");
		command->SetRequest(m_szRequest);
		command->SetProtocol(rpXmlRpc);
		command->PrepareParams();
		command->Execute();
		BuildResponse(command->GetResponse(), "", command->GetFault());
		delete command;
	}
	else
	{
		cStringBuilder.Append("</data></array>");
		BuildResponse(cStringBuilder.GetBuffer(), "", false);
	}
}

void XmlRpcProcessor::BuildResponse(const char* szResponse, const char* szCallbackFunc, bool bFault)
{
	const char XML_HEADER[] = "<?xml version=\"1.0\"?>\n<methodResponse>\n";
	const char XML_FOOTER[] = "</methodResponse>";
	const char XML_OK_OPEN[] = "<params><param><value>";
	const char XML_OK_CLOSE[] = "</value></param></params>\n";
	const char XML_FAULT_OPEN[] = "<fault><value>";
	const char XML_FAULT_CLOSE[] = "</value></fault>\n";

	const char JSON_HEADER[] = "{\n\"version\" : \"1.1\",\n";
	const char JSON_FOOTER[] = "\n}";
	const char JSON_OK_OPEN[] = "\"result\" : ";
	const char JSON_OK_CLOSE[] = "";
	const char JSON_FAULT_OPEN[] = "\"error\" : ";
	const char JSON_FAULT_CLOSE[] = "";

	const char JSONP_CALLBACK_HEADER[] = "(";
	const char JSONP_CALLBACK_FOOTER[] = ")";

	bool bXmlRpc = m_eProtocol == rpXmlRpc;

	const char* szCallbackHeader = m_eProtocol == rpJsonPRpc ? JSONP_CALLBACK_HEADER : "";
	const char* szHeader = bXmlRpc ? XML_HEADER : JSON_HEADER;
	const char* szFooter = bXmlRpc ? XML_FOOTER : JSON_FOOTER;
	const char* szOpenTag = bFault ? (bXmlRpc ? XML_FAULT_OPEN : JSON_FAULT_OPEN) : (bXmlRpc ? XML_OK_OPEN : JSON_OK_OPEN);
	const char* szCloseTag = bFault ? (bXmlRpc ? XML_FAULT_CLOSE : JSON_FAULT_CLOSE ) : (bXmlRpc ? XML_OK_CLOSE : JSON_OK_CLOSE);
	const char* szCallbackFooter = m_eProtocol == rpJsonPRpc ? JSONP_CALLBACK_FOOTER : "";

	debug("Response=%s", szResponse);

	if (szCallbackFunc)
	{
		m_cResponse.Append(szCallbackFunc);
	}
	m_cResponse.Append(szCallbackHeader);
	m_cResponse.Append(szHeader);
	m_cResponse.Append(szOpenTag);
	m_cResponse.Append(szResponse);
	m_cResponse.Append(szCloseTag);
	m_cResponse.Append(szFooter);
	m_cResponse.Append(szCallbackFooter);
	
	m_szContentType = bXmlRpc ? "text/xml" : "application/json";
}

XmlCommand* XmlRpcProcessor::CreateCommand(const char* szMethodName)
{
	XmlCommand* command = NULL;

	if (!strcasecmp(szMethodName, "pause") || !strcasecmp(szMethodName, "pausedownload") ||
		!strcasecmp(szMethodName, "pausedownload2"))
	{
		command = new PauseUnpauseXmlCommand(true, PauseUnpauseXmlCommand::paDownload);
	}
	else if (!strcasecmp(szMethodName, "resume") || !strcasecmp(szMethodName, "resumedownload") ||
		!strcasecmp(szMethodName, "resumedownload2"))
	{
		command = new PauseUnpauseXmlCommand(false, PauseUnpauseXmlCommand::paDownload);
	}
	else if (!strcasecmp(szMethodName, "shutdown"))
	{
		command = new ShutdownXmlCommand();
	}
	else if (!strcasecmp(szMethodName, "reload"))
	{
		command = new ReloadXmlCommand();
	}
	else if (!strcasecmp(szMethodName, "version"))
	{
		command = new VersionXmlCommand();
	}
	else if (!strcasecmp(szMethodName, "dump"))
	{
		command = new DumpDebugXmlCommand();
	}
	else if (!strcasecmp(szMethodName, "rate"))
	{
		command = new SetDownloadRateXmlCommand();
	}
	else if (!strcasecmp(szMethodName, "status"))
	{
		command = new StatusXmlCommand();
	}
	else if (!strcasecmp(szMethodName, "log"))
	{
		command = new LogXmlCommand();
	}
	else if (!strcasecmp(szMethodName, "listfiles"))
	{
		command = new ListFilesXmlCommand();
	}
	else if (!strcasecmp(szMethodName, "listgroups"))
	{
		command = new ListGroupsXmlCommand();
	}
	else if (!strcasecmp(szMethodName, "editqueue"))
	{
		command = new EditQueueXmlCommand();
	}
	else if (!strcasecmp(szMethodName, "append") || !strcasecmp(szMethodName, "appendurl"))
	{
		command = new DownloadXmlCommand();
	}
	else if (!strcasecmp(szMethodName, "postqueue"))
	{
		command = new PostQueueXmlCommand();
	}
	else if (!strcasecmp(szMethodName, "writelog"))
	{
		command = new WriteLogXmlCommand();
	}
	else if (!strcasecmp(szMethodName, "clearlog"))
	{
		command = new ClearLogXmlCommand();
	}
	else if (!strcasecmp(szMethodName, "scan"))
	{
		command = new ScanXmlCommand();
	}
	else if (!strcasecmp(szMethodName, "pausepost"))
	{
		command = new PauseUnpauseXmlCommand(true, PauseUnpauseXmlCommand::paPostProcess);
	}
	else if (!strcasecmp(szMethodName, "resumepost"))
	{
		command = new PauseUnpauseXmlCommand(false, PauseUnpauseXmlCommand::paPostProcess);
	}
	else if (!strcasecmp(szMethodName, "pausescan"))
	{
		command = new PauseUnpauseXmlCommand(true, PauseUnpauseXmlCommand::paScan);
	}
	else if (!strcasecmp(szMethodName, "resumescan"))
	{
		command = new PauseUnpauseXmlCommand(false, PauseUnpauseXmlCommand::paScan);
	}
	else if (!strcasecmp(szMethodName, "scheduleresume"))
	{
		command = new ScheduleResumeXmlCommand();
	}
	else if (!strcasecmp(szMethodName, "history"))
	{
		command = new HistoryXmlCommand();
	}
	else if (!strcasecmp(szMethodName, "urlqueue"))
	{
		command = new UrlQueueXmlCommand();
	}
	else if (!strcasecmp(szMethodName, "config"))
	{
		command = new ConfigXmlCommand();
	}
	else if (!strcasecmp(szMethodName, "loadconfig"))
	{
		command = new LoadConfigXmlCommand();
	}
	else if (!strcasecmp(szMethodName, "saveconfig"))
	{
		command = new SaveConfigXmlCommand();
	}
	else if (!strcasecmp(szMethodName, "configtemplates"))
	{
		command = new ConfigTemplatesXmlCommand();
	}
	else if (!strcasecmp(szMethodName, "viewfeed"))
	{
		command = new ViewFeedXmlCommand(false);
	}
	else if (!strcasecmp(szMethodName, "previewfeed"))
	{
		command = new ViewFeedXmlCommand(true);
	}
	else if (!strcasecmp(szMethodName, "fetchfeed"))
	{
		command = new FetchFeedXmlCommand();
	}
	else if (!strcasecmp(szMethodName, "editserver"))
	{
		command = new EditServerXmlCommand();
	}
	else if (!strcasecmp(szMethodName, "readurl"))
	{
		command = new ReadUrlXmlCommand();
	}
	else if (!strcasecmp(szMethodName, "checkupdates"))
	{
		command = new CheckUpdatesXmlCommand();
	}
	else if (!strcasecmp(szMethodName, "startupdate"))
	{
		command = new StartUpdateXmlCommand();
	}
	else if (!strcasecmp(szMethodName, "logupdate"))
	{
		command = new LogUpdateXmlCommand();
	}
	else if (!strcasecmp(szMethodName, "servervolumes"))
	{
		command = new ServerVolumesXmlCommand();
	}
	else if (!strcasecmp(szMethodName, "resetservervolume"))
	{
		command = new ResetServerVolumeXmlCommand();
	}
	else
	{
		command = new ErrorXmlCommand(1, "Invalid procedure");
	}

	return command;
}


//*****************************************************************
// Base command

XmlCommand::XmlCommand()
{
	m_szRequest = NULL;
	m_szRequestPtr = NULL;
	m_szCallbackFunc = NULL;
	m_bFault = false;
	m_eProtocol = XmlRpcProcessor::rpUndefined;
}

bool XmlCommand::IsJson()
{ 
	return m_eProtocol == XmlRpcProcessor::rpJsonRpc || m_eProtocol == XmlRpcProcessor::rpJsonPRpc;
}

void XmlCommand::AppendResponse(const char* szPart)
{
	m_StringBuilder.Append(szPart);
}

void XmlCommand::BuildErrorResponse(int iErrCode, const char* szErrText, ...)
{
	const char* XML_RESPONSE_ERROR_BODY = 
		"<struct>\n"
		"<member><name>faultCode</name><value><i4>%i</i4></value></member>\n"
		"<member><name>faultString</name><value><string>%s</string></value></member>\n"
		"</struct>\n";

	const char* JSON_RESPONSE_ERROR_BODY = 
		"{\n"
        "\"name\" : \"JSONRPCError\",\n"
        "\"code\" : %i,\n"
        "\"message\" : \"%s\"\n"
        "}";

	char szFullText[1024];

	va_list ap;
	va_start(ap, szErrText);
	vsnprintf(szFullText, 1024, szErrText, ap);
	szFullText[1024-1] = '\0';
	va_end(ap);

	char* xmlText = EncodeStr(szFullText);

	char szContent[1024];
	snprintf(szContent, 1024, IsJson() ? JSON_RESPONSE_ERROR_BODY : XML_RESPONSE_ERROR_BODY, iErrCode, xmlText);
	szContent[1024-1] = '\0';

	free(xmlText);

	AppendResponse(szContent);

	m_bFault = true;
}

void XmlCommand::BuildBoolResponse(bool bOK)
{
	const char* XML_RESPONSE_BOOL_BODY = "<boolean>%s</boolean>";
	const char* JSON_RESPONSE_BOOL_BODY = "%s";

	char szContent[1024];
	snprintf(szContent, 1024, IsJson() ? JSON_RESPONSE_BOOL_BODY : XML_RESPONSE_BOOL_BODY,
		BoolToStr(bOK));
	szContent[1024-1] = '\0';

	AppendResponse(szContent);
}

void XmlCommand::BuildIntResponse(int iValue)
{
	const char* XML_RESPONSE_INT_BODY = "<i4>%i</i4>";
	const char* JSON_RESPONSE_INT_BODY = "%i";

	char szContent[1024];
	snprintf(szContent, 1024, IsJson() ? JSON_RESPONSE_INT_BODY : XML_RESPONSE_INT_BODY, iValue);
	szContent[1024-1] = '\0';

	AppendResponse(szContent);
}

void XmlCommand::PrepareParams()
{
	if (IsJson() && m_eHttpMethod == XmlRpcProcessor::hmPost)
	{
		char* szParams = strstr(m_szRequestPtr, "\"params\"");
		if (!szParams)
		{
			m_szRequestPtr[0] = '\0';
			return;
		}
		m_szRequestPtr = szParams + 8; // strlen("\"params\"")
	}

	if (m_eProtocol == XmlRpcProcessor::rpJsonPRpc)
	{
		NextParamAsStr(&m_szCallbackFunc);
	}
}

char* XmlCommand::XmlNextValue(char* szXml, const char* szTag, int* pValueLength)
{
	int iValueLen;
	const char* szValue = WebUtil::XmlFindTag(szXml, "value", &iValueLen);
	if (szValue)
	{
		char* szTagContent = (char*)WebUtil::XmlFindTag(szValue, szTag, pValueLength);
		if (szTagContent <= szValue + iValueLen)
		{
			return szTagContent;
		}
	}
	return NULL;
}

bool XmlCommand::NextParamAsInt(int* iValue)
{
	if (m_eHttpMethod == XmlRpcProcessor::hmGet)
	{
		char* szParam = strchr(m_szRequestPtr, '=');
		if (!szParam)
		{
			return false;
		}
		*iValue = atoi(szParam + 1);
		m_szRequestPtr = szParam + 1;
		return true;
	}
	else if (IsJson())
	{
		int iLen = 0;
		char* szParam = (char*)WebUtil::JsonNextValue(m_szRequestPtr, &iLen);
		if (!szParam || !strchr("-+0123456789", *szParam))
		{
			return false;
		}
		*iValue = atoi(szParam);
		m_szRequestPtr = szParam + iLen + 1;
		return true;
	}
	else
	{
		int iLen = 0;
		int iTagLen = 4; //strlen("<i4>");
		char* szParam = XmlNextValue(m_szRequestPtr, "i4", &iLen);
		if (!szParam)
		{
			szParam = XmlNextValue(m_szRequestPtr, "int", &iLen);
			iTagLen = 5; //strlen("<int>");
		}
		if (!szParam || !strchr("-+0123456789", *szParam))
		{
			return false;
		}
		*iValue = atoi(szParam);
		m_szRequestPtr = szParam + iLen + iTagLen;
		return true;
	}
}

bool XmlCommand::NextParamAsBool(bool* bValue)
{
	if (m_eHttpMethod == XmlRpcProcessor::hmGet)
	{
		char* szParam;
		if (!NextParamAsStr(&szParam))
		{
			return false;
		}

		if (IsJson())
		{
			if (!strcmp(szParam, "true"))
			{
				*bValue = true;
				return true;
			}
			else if (!strcmp(szParam, "false"))
			{
				*bValue = false;
				return true;
			}
		}
		else
		{
			*bValue = szParam[0] == '1';
			return true;
		}
		return false;
	}
	else if (IsJson())
	{
		int iLen = 0;
		char* szParam = (char*)WebUtil::JsonNextValue(m_szRequestPtr, &iLen);
		if (!szParam)
		{
			return false;
		}
		if (iLen == 4 && !strncmp(szParam, "true", 4))
		{
			*bValue = true;
			m_szRequestPtr = szParam + iLen + 1;
			return true;
		}
		else if (iLen == 5 && !strncmp(szParam, "false", 5))
		{
			*bValue = false;
			m_szRequestPtr = szParam + iLen + 1;
			return true;
		}
		else
		{
			return false;
		}
	}
	else
	{
		int iLen = 0;
		char* szParam = XmlNextValue(m_szRequestPtr, "boolean", &iLen);
		if (!szParam)
		{
			return false;
		}
		*bValue = szParam[0] == '1';
		m_szRequestPtr = szParam + iLen + 9; //strlen("<boolean>");
		return true;
	}
}

bool XmlCommand::NextParamAsStr(char** szValue)
{
	if (m_eHttpMethod == XmlRpcProcessor::hmGet)
	{
		char* szParam = strchr(m_szRequestPtr, '=');
		if (!szParam)
		{
			return false;
		}
		szParam++; // skip '='
		int iLen = 0;
		char* szParamEnd = strchr(m_szRequestPtr, '&');
		if (szParamEnd)
		{
			iLen = (int)(szParamEnd - szParam);
			szParam[iLen] = '\0';
		}
		else
		{
			iLen = strlen(szParam) - 1;
		}
		m_szRequestPtr = szParam + iLen + 1;
		*szValue = szParam;
		return true;
	}
	else if (IsJson())
	{
		int iLen = 0;
		char* szParam = (char*)WebUtil::JsonNextValue(m_szRequestPtr, &iLen);
		if (!szParam || iLen < 2 || szParam[0] != '"' || szParam[iLen - 1] != '"')
		{
			return false;
		}
		szParam++; // skip first '"'
		szParam[iLen - 2] = '\0'; // skip last '"'
		m_szRequestPtr = szParam + iLen;
		*szValue = szParam;
		return true;
	}
	else
	{
		int iLen = 0;
		char* szParam = XmlNextValue(m_szRequestPtr, "string", &iLen);
		if (!szParam)
		{
			return false;
		}
		szParam[iLen] = '\0';
		m_szRequestPtr = szParam + iLen + 8; //strlen("<string>")
		*szValue = szParam;
		return true;
	}
}

const char* XmlCommand::BoolToStr(bool bValue)
{
	return IsJson() ? (bValue ? "true" : "false") : (bValue ? "1" : "0");
}

char* XmlCommand::EncodeStr(const char* szStr)
{
	if (!szStr)
	{
		return strdup("");
	}

	if (IsJson()) 
	{
		return WebUtil::JsonEncode(szStr);
	}
	else
	{
		return WebUtil::XmlEncode(szStr);
	}
}

void XmlCommand::DecodeStr(char* szStr)
{
	if (IsJson())
	{
		WebUtil::JsonDecode(szStr);
	}
	else
	{
		WebUtil::XmlDecode(szStr);
	}
}

bool XmlCommand::CheckSafeMethod()
{
	bool bSafe = m_eHttpMethod == XmlRpcProcessor::hmPost || m_eProtocol == XmlRpcProcessor::rpJsonPRpc;
	if (!bSafe)
	{
		BuildErrorResponse(4, "Not safe procedure for HTTP-Method GET. Use Method POST instead");
	}
	return bSafe;
}

//*****************************************************************
// Commands

ErrorXmlCommand::ErrorXmlCommand(int iErrCode, const char* szErrText)
{
	m_iErrCode = iErrCode;
	m_szErrText = szErrText;
}

void ErrorXmlCommand::Execute()
{
	error("Received unsupported request: %s", m_szErrText);
	BuildErrorResponse(m_iErrCode, m_szErrText);
}

PauseUnpauseXmlCommand::PauseUnpauseXmlCommand(bool bPause, EPauseAction eEPauseAction)
{
	m_bPause = bPause;
	m_eEPauseAction = eEPauseAction;
}

void PauseUnpauseXmlCommand::Execute()
{
	if (!CheckSafeMethod())
	{
		return;
	}

	bool bOK = true;

	g_pOptions->SetResumeTime(0);

	switch (m_eEPauseAction)
	{
		case paDownload:
			g_pOptions->SetPauseDownload(m_bPause);
			break;

		case paPostProcess:
			g_pOptions->SetPausePostProcess(m_bPause);
			break;

		case paScan:
			g_pOptions->SetPauseScan(m_bPause);
			break;

		default:
			bOK = false;
	}

	BuildBoolResponse(bOK);
}

// bool scheduleresume(int Seconds) 
void ScheduleResumeXmlCommand::Execute()
{
	if (!CheckSafeMethod())
	{
		return;
	}

	int iSeconds = 0;
	if (!NextParamAsInt(&iSeconds) || iSeconds < 0)
	{
		BuildErrorResponse(2, "Invalid parameter");
		return;
	}

	time_t tCurTime = time(NULL);

	g_pOptions->SetResumeTime(tCurTime + iSeconds);

	BuildBoolResponse(true);
}

void ShutdownXmlCommand::Execute()
{
	if (!CheckSafeMethod())
	{
		return;
	}

	BuildBoolResponse(true);
	ExitProc();
}

void ReloadXmlCommand::Execute()
{
	if (!CheckSafeMethod())
	{
		return;
	}

	BuildBoolResponse(true);
	Reload();
}

void VersionXmlCommand::Execute()
{
	const char* XML_RESPONSE_STRING_BODY = "<string>%s</string>";
	const char* JSON_RESPONSE_STRING_BODY = "\"%s\"";

	char szContent[1024];
	snprintf(szContent, 1024, IsJson() ? JSON_RESPONSE_STRING_BODY : XML_RESPONSE_STRING_BODY, Util::VersionRevision());
	szContent[1024-1] = '\0';

	AppendResponse(szContent);
}

void DumpDebugXmlCommand::Execute()
{
	g_pLog->LogDebugInfo();
	BuildBoolResponse(true);
}

void SetDownloadRateXmlCommand::Execute()
{
	if (!CheckSafeMethod())
	{
		return;
	}

	int iRate = 0;
	if (!NextParamAsInt(&iRate) || iRate < 0)
	{
		BuildErrorResponse(2, "Invalid parameter");
		return;
	}

	g_pOptions->SetDownloadRate(iRate * 1024);
	BuildBoolResponse(true);
}

void StatusXmlCommand::Execute()
{
	const char* XML_STATUS_START = 
		"<struct>\n"
		"<member><name>RemainingSizeLo</name><value><i4>%u</i4></value></member>\n"
		"<member><name>RemainingSizeHi</name><value><i4>%u</i4></value></member>\n"
		"<member><name>RemainingSizeMB</name><value><i4>%i</i4></value></member>\n"
		"<member><name>ForcedSizeLo</name><value><i4>%u</i4></value></member>\n"
		"<member><name>ForcedSizeHi</name><value><i4>%u</i4></value></member>\n"
		"<member><name>ForcedSizeMB</name><value><i4>%i</i4></value></member>\n"
		"<member><name>DownloadedSizeLo</name><value><i4>%u</i4></value></member>\n"
		"<member><name>DownloadedSizeHi</name><value><i4>%u</i4></value></member>\n"
		"<member><name>DownloadedSizeMB</name><value><i4>%i</i4></value></member>\n"
		"<member><name>ArticleCacheLo</name><value><i4>%u</i4></value></member>\n"
		"<member><name>ArticleCacheHi</name><value><i4>%u</i4></value></member>\n"
		"<member><name>ArticleCacheMB</name><value><i4>%i</i4></value></member>\n"
		"<member><name>DownloadRate</name><value><i4>%i</i4></value></member>\n"
		"<member><name>AverageDownloadRate</name><value><i4>%i</i4></value></member>\n"
		"<member><name>DownloadLimit</name><value><i4>%i</i4></value></member>\n"
		"<member><name>ThreadCount</name><value><i4>%i</i4></value></member>\n"
		"<member><name>ParJobCount</name><value><i4>%i</i4></value></member>\n"					// deprecated (renamed to PostJobCount)
		"<member><name>PostJobCount</name><value><i4>%i</i4></value></member>\n"
		"<member><name>UrlCount</name><value><i4>%i</i4></value></member>\n"
		"<member><name>UpTimeSec</name><value><i4>%i</i4></value></member>\n"
		"<member><name>DownloadTimeSec</name><value><i4>%i</i4></value></member>\n"
		"<member><name>ServerPaused</name><value><boolean>%s</boolean></value></member>\n"		// deprecated (renamed to DownloadPaused)
		"<member><name>DownloadPaused</name><value><boolean>%s</boolean></value></member>\n"
		"<member><name>Download2Paused</name><value><boolean>%s</boolean></value></member>\n"	// deprecated (same as DownloadPaused)
		"<member><name>ServerStandBy</name><value><boolean>%s</boolean></value></member>\n"
		"<member><name>PostPaused</name><value><boolean>%s</boolean></value></member>\n"
		"<member><name>ScanPaused</name><value><boolean>%s</boolean></value></member>\n"
		"<member><name>FreeDiskSpaceLo</name><value><i4>%u</i4></value></member>\n"
		"<member><name>FreeDiskSpaceHi</name><value><i4>%u</i4></value></member>\n"
		"<member><name>FreeDiskSpaceMB</name><value><i4>%i</i4></value></member>\n"
		"<member><name>ServerTime</name><value><i4>%i</i4></value></member>\n"
		"<member><name>ResumeTime</name><value><i4>%i</i4></value></member>\n"
		"<member><name>FeedActive</name><value><boolean>%s</boolean></value></member>\n"
		"<member><name>NewsServers</name><value><array><data>\n";

	const char* XML_STATUS_END =
		"</data></array></value></member>\n"
		"</struct>\n";

	const char* JSON_STATUS_START = 
		"{\n"
		"\"RemainingSizeLo\" : %u,\n"
		"\"RemainingSizeHi\" : %u,\n"
		"\"RemainingSizeMB\" : %i,\n"
		"\"ForcedSizeLo\" : %u,\n"
		"\"ForcedSizeHi\" : %u,\n"
		"\"ForcedSizeMB\" : %i,\n"
		"\"DownloadedSizeLo\" : %u,\n"
		"\"DownloadedSizeHi\" : %u,\n"
		"\"DownloadedSizeMB\" : %i,\n"
		"\"ArticleCacheLo\" : %u,\n"
		"\"ArticleCacheHi\" : %u,\n"
		"\"ArticleCacheMB\" : %i,\n"
		"\"DownloadRate\" : %i,\n"
		"\"AverageDownloadRate\" : %i,\n"
		"\"DownloadLimit\" : %i,\n"
		"\"ThreadCount\" : %i,\n"
		"\"ParJobCount\" : %i,\n"			// deprecated (renamed to PostJobCount)
		"\"PostJobCount\" : %i,\n"
		"\"UrlCount\" : %i,\n"
		"\"UpTimeSec\" : %i,\n"
		"\"DownloadTimeSec\" : %i,\n"
		"\"ServerPaused\" : %s,\n"			// deprecated (renamed to DownloadPaused)
		"\"DownloadPaused\" : %s,\n"
		"\"Download2Paused\" : %s,\n"		// deprecated (same as DownloadPaused)
		"\"ServerStandBy\" : %s,\n"
		"\"PostPaused\" : %s,\n"
		"\"ScanPaused\" : %s,\n"
		"\"FreeDiskSpaceLo\" : %u,\n"
		"\"FreeDiskSpaceHi\" : %u,\n"
		"\"FreeDiskSpaceMB\" : %i,\n"
		"\"ServerTime\" : %i,\n"
		"\"ResumeTime\" : %i,\n"
		"\"FeedActive\" : %s,\n"
		"\"NewsServers\" : [\n";

	const char* JSON_STATUS_END = 
		"]\n"
		"}";

	const char* XML_NEWSSERVER_ITEM = 
		"<value><struct>\n"
		"<member><name>ID</name><value><i4>%i</i4></value></member>\n"
		"<member><name>Active</name><value><boolean>%s</boolean></value></member>\n"
		"</struct></value>\n";

	const char* JSON_NEWSSERVER_ITEM = 
		"{\n"
		"\"ID\" : %i,\n"
		"\"Active\" : %s\n"
		"}";

	DownloadQueue *pDownloadQueue = DownloadQueue::Lock();
	int iPostJobCount = 0;
	int iUrlCount = 0;
	for (NZBList::iterator it = pDownloadQueue->GetQueue()->begin(); it != pDownloadQueue->GetQueue()->end(); it++)
	{
		NZBInfo* pNZBInfo = *it;
		iPostJobCount += pNZBInfo->GetPostInfo() ? 1 : 0;
		iUrlCount += pNZBInfo->GetKind() == NZBInfo::nkUrl ? 1 : 0;
	}
	long long iRemainingSize, iForcedSize;
	pDownloadQueue->CalcRemainingSize(&iRemainingSize, &iForcedSize);
	DownloadQueue::Unlock();

	unsigned long iRemainingSizeHi, iRemainingSizeLo;
	Util::SplitInt64(iRemainingSize, &iRemainingSizeHi, &iRemainingSizeLo);
	int iRemainingMBytes = (int)(iRemainingSize / 1024 / 1024);

	unsigned long iForcedSizeHi, iForcedSizeLo;
	Util::SplitInt64(iForcedSize, &iForcedSizeHi, &iForcedSizeLo);
	int iForcedMBytes = (int)(iForcedSize / 1024 / 1024);

	long long iArticleCache = g_pArticleCache->GetAllocated();
	unsigned long iArticleCacheHi, iArticleCacheLo;
	Util::SplitInt64(iArticleCache, &iArticleCacheHi, &iArticleCacheLo);
	int iArticleCacheMBytes = (int)(iArticleCache / 1024 / 1024);

	int iDownloadRate = (int)(g_pStatMeter->CalcCurrentDownloadSpeed());
	int iDownloadLimit = (int)(g_pOptions->GetDownloadRate());
	bool bDownloadPaused = g_pOptions->GetPauseDownload();
	bool bPostPaused = g_pOptions->GetPausePostProcess();
	bool bScanPaused = g_pOptions->GetPauseScan();
	int iThreadCount = Thread::GetThreadCount() - 1; // not counting itself

	unsigned long iDownloadedSizeHi, iDownloadedSizeLo;
	int iUpTimeSec, iDownloadTimeSec;
	long long iAllBytes;
	bool bServerStandBy;
	g_pStatMeter->CalcTotalStat(&iUpTimeSec, &iDownloadTimeSec, &iAllBytes, &bServerStandBy);
	int iDownloadedMBytes = (int)(iAllBytes / 1024 / 1024);
	Util::SplitInt64(iAllBytes, &iDownloadedSizeHi, &iDownloadedSizeLo);
	int iAverageDownloadRate = (int)(iDownloadTimeSec > 0 ? iAllBytes / iDownloadTimeSec : 0);
	unsigned long iFreeDiskSpaceHi, iFreeDiskSpaceLo;
	long long iFreeDiskSpace = Util::FreeDiskSize(g_pOptions->GetDestDir());
	Util::SplitInt64(iFreeDiskSpace, &iFreeDiskSpaceHi, &iFreeDiskSpaceLo);
	int iFreeDiskSpaceMB = (int)(iFreeDiskSpace / 1024 / 1024);
	int iServerTime = time(NULL);
	int iResumeTime = g_pOptions->GetResumeTime();
	bool bFeedActive = g_pFeedCoordinator->HasActiveDownloads();
	
	char szContent[3072];
	snprintf(szContent, 3072, IsJson() ? JSON_STATUS_START : XML_STATUS_START, 
		iRemainingSizeLo, iRemainingSizeHi, iRemainingMBytes, iForcedSizeLo,
		iForcedSizeHi, iForcedMBytes, iDownloadedSizeLo, iDownloadedSizeHi,
		iDownloadedMBytes, iArticleCacheLo, iArticleCacheHi, iArticleCacheMBytes,
		iDownloadRate, iAverageDownloadRate, iDownloadLimit, iThreadCount, 
		iPostJobCount, iPostJobCount, iUrlCount, iUpTimeSec, iDownloadTimeSec, 
		BoolToStr(bDownloadPaused), BoolToStr(bDownloadPaused), BoolToStr(bDownloadPaused), 
		BoolToStr(bServerStandBy), BoolToStr(bPostPaused), BoolToStr(bScanPaused),
		iFreeDiskSpaceLo, iFreeDiskSpaceHi,	iFreeDiskSpaceMB, iServerTime, iResumeTime,
		BoolToStr(bFeedActive));
	szContent[3072-1] = '\0';

	AppendResponse(szContent);

	int index = 0;
	for (Servers::iterator it = g_pServerPool->GetServers()->begin(); it != g_pServerPool->GetServers()->end(); it++)
	{
		NewsServer* pServer = *it;
		snprintf(szContent, sizeof(szContent), IsJson() ? JSON_NEWSSERVER_ITEM : XML_NEWSSERVER_ITEM,
			pServer->GetID(), BoolToStr(pServer->GetActive()));
		szContent[3072-1] = '\0';

		if (IsJson() && index++ > 0)
		{
			AppendResponse(",\n");
		}
		AppendResponse(szContent);
	}

	AppendResponse(IsJson() ? JSON_STATUS_END : XML_STATUS_END);
}

// struct[] log(idfrom, entries)
void LogXmlCommand::Execute()
{
	int iIDFrom = 0;
	int iNrEntries = 0;
	if (!NextParamAsInt(&iIDFrom) || !NextParamAsInt(&iNrEntries) || (iNrEntries > 0 && iIDFrom > 0))
	{
		BuildErrorResponse(2, "Invalid parameter");
		return;
	}

	debug("iIDFrom=%i", iIDFrom);
	debug("iNrEntries=%i", iNrEntries);

	AppendResponse(IsJson() ? "[\n" : "<array><data>\n");
	Log::Messages* pMessages = LockMessages();

	int iStart = pMessages->size();
	if (iNrEntries > 0)
	{
		if (iNrEntries > (int)pMessages->size())
		{
			iNrEntries = pMessages->size();
		}
		iStart = pMessages->size() - iNrEntries;
	}
	if (iIDFrom > 0 && !pMessages->empty())
	{
		iNrEntries = pMessages->size();
		iStart = iIDFrom - pMessages->front()->GetID();
		if (iStart < 0)
		{
			iStart = 0;
		}
	}

	const char* XML_LOG_ITEM = 
		"<value><struct>\n"
		"<member><name>ID</name><value><i4>%i</i4></value></member>\n"
		"<member><name>Kind</name><value><string>%s</string></value></member>\n"
		"<member><name>Time</name><value><i4>%i</i4></value></member>\n"
		"<member><name>Text</name><value><string>%s</string></value></member>\n"
		"</struct></value>\n";

	const char* JSON_LOG_ITEM = 
		"{\n"
		"\"ID\" : %i,\n"
		"\"Kind\" : \"%s\",\n"
		"\"Time\" : %i,\n"
		"\"Text\" : \"%s\"\n"
		"}";

    const char* szMessageType[] = { "INFO", "WARNING", "ERROR", "DEBUG", "DETAIL" };

	int iItemBufSize = 10240;
	char* szItemBuf = (char*)malloc(iItemBufSize);
	int index = 0;

	for (unsigned int i = (unsigned int)iStart; i < pMessages->size(); i++)
	{
		Message* pMessage = (*pMessages)[i];
		char* xmltext = EncodeStr(pMessage->GetText());
		snprintf(szItemBuf, iItemBufSize, IsJson() ? JSON_LOG_ITEM : XML_LOG_ITEM,
			pMessage->GetID(), szMessageType[pMessage->GetKind()], pMessage->GetTime(), xmltext);
		szItemBuf[iItemBufSize-1] = '\0';
		free(xmltext);

		if (IsJson() && index++ > 0)
		{
			AppendResponse(",\n");
		}
		AppendResponse(szItemBuf);
	}

	free(szItemBuf);

	UnlockMessages();
	AppendResponse(IsJson() ? "\n]" : "</data></array>\n");
}

Log::Messages* LogXmlCommand::LockMessages()
{
	return g_pLog->LockMessages();
}

void LogXmlCommand::UnlockMessages()
{
	g_pLog->UnlockMessages();
}

// struct[] listfiles(int IDFrom, int IDTo, int NZBID) 
// For backward compatibility with 0.8 parameter "NZBID" is optional
void ListFilesXmlCommand::Execute()
{
	int iIDStart = 0;
	int iIDEnd = 0;
	if (NextParamAsInt(&iIDStart) && (!NextParamAsInt(&iIDEnd) || iIDEnd < iIDStart))
	{
		BuildErrorResponse(2, "Invalid parameter");
		return;
	}

	// For backward compatibility with 0.8 parameter "NZBID" is optional (error checking omitted)
	int iNZBID = 0;
	NextParamAsInt(&iNZBID);

	if (iNZBID > 0 && (iIDStart != 0 || iIDEnd != 0))
	{
		BuildErrorResponse(2, "Invalid parameter");
		return;
	}

	debug("iIDStart=%i", iIDStart);
	debug("iIDEnd=%i", iIDEnd);

	AppendResponse(IsJson() ? "[\n" : "<array><data>\n");
	DownloadQueue* pDownloadQueue = DownloadQueue::Lock();

	const char* XML_LIST_ITEM = 
		"<value><struct>\n"
		"<member><name>ID</name><value><i4>%i</i4></value></member>\n"
		"<member><name>FileSizeLo</name><value><i4>%u</i4></value></member>\n"
		"<member><name>FileSizeHi</name><value><i4>%u</i4></value></member>\n"
		"<member><name>RemainingSizeLo</name><value><i4>%u</i4></value></member>\n"
		"<member><name>RemainingSizeHi</name><value><i4>%u</i4></value></member>\n"
		"<member><name>PostTime</name><value><i4>%i</i4></value></member>\n"
		"<member><name>FilenameConfirmed</name><value><boolean>%s</boolean></value></member>\n"
		"<member><name>Paused</name><value><boolean>%s</boolean></value></member>\n"
		"<member><name>NZBID</name><value><i4>%i</i4></value></member>\n"
		"<member><name>NZBName</name><value><string>%s</string></value></member>\n"
		"<member><name>NZBNicename</name><value><string>%s</string></value></member>\n"	// deprecated, use "NZBName" instead
		"<member><name>NZBFilename</name><value><string>%s</string></value></member>\n"
		"<member><name>Subject</name><value><string>%s</string></value></member>\n"
		"<member><name>Filename</name><value><string>%s</string></value></member>\n"
		"<member><name>DestDir</name><value><string>%s</string></value></member>\n"
		"<member><name>Category</name><value><string>%s</string></value></member>\n"
		"<member><name>Priority</name><value><i4>%i</i4></value></member>\n"			// deprecated, use "Priority" of group instead
		"<member><name>ActiveDownloads</name><value><i4>%i</i4></value></member>\n"
		"</struct></value>\n";

	const char* JSON_LIST_ITEM = 
		"{\n"
		"\"ID\" : %i,\n"
		"\"FileSizeLo\" : %u,\n"
		"\"FileSizeHi\" : %u,\n"
		"\"RemainingSizeLo\" : %u,\n"
		"\"RemainingSizeHi\" : %u,\n"
		"\"PostTime\" : %i,\n"
		"\"FilenameConfirmed\" : %s,\n"
		"\"Paused\" : %s,\n"
		"\"NZBID\" : %i,\n"
		"\"NZBName\" : \"%s\",\n"
		"\"NZBNicename\" : \"%s\",\n" 		// deprecated, use "NZBName" instead
		"\"NZBFilename\" : \"%s\",\n"
		"\"Subject\" : \"%s\",\n"
		"\"Filename\" : \"%s\",\n"
		"\"DestDir\" : \"%s\",\n"
		"\"Category\" : \"%s\",\n"
		"\"Priority\" : %i,\n"				// deprecated, use "Priority" of group instead
		"\"ActiveDownloads\" : %i\n"
		"}";

	int iItemBufSize = 10240;
	char* szItemBuf = (char*)malloc(iItemBufSize);
	int index = 0;

	for (NZBList::iterator it = pDownloadQueue->GetQueue()->begin(); it != pDownloadQueue->GetQueue()->end(); it++)
	{
		NZBInfo* pNZBInfo = *it;
		for (FileList::iterator it2 = pNZBInfo->GetFileList()->begin(); it2 != pNZBInfo->GetFileList()->end(); it2++)
		{
			FileInfo* pFileInfo = *it2;

			if ((iNZBID > 0 && iNZBID == pFileInfo->GetNZBInfo()->GetID()) ||
				(iNZBID == 0 && (iIDStart == 0 || (iIDStart <= pFileInfo->GetID() && pFileInfo->GetID() <= iIDEnd))))
			{
				unsigned long iFileSizeHi, iFileSizeLo;
				unsigned long iRemainingSizeLo, iRemainingSizeHi;
				Util::SplitInt64(pFileInfo->GetSize(), &iFileSizeHi, &iFileSizeLo);
				Util::SplitInt64(pFileInfo->GetRemainingSize(), &iRemainingSizeHi, &iRemainingSizeLo);
				char* xmlNZBFilename = EncodeStr(pFileInfo->GetNZBInfo()->GetFilename());
				char* xmlSubject = EncodeStr(pFileInfo->GetSubject());
				char* xmlFilename = EncodeStr(pFileInfo->GetFilename());
				char* xmlDestDir = EncodeStr(pFileInfo->GetNZBInfo()->GetDestDir());
				char* xmlCategory = EncodeStr(pFileInfo->GetNZBInfo()->GetCategory());
				char* xmlNZBNicename = EncodeStr(pFileInfo->GetNZBInfo()->GetName());

				snprintf(szItemBuf, iItemBufSize, IsJson() ? JSON_LIST_ITEM : XML_LIST_ITEM,
					pFileInfo->GetID(), iFileSizeLo, iFileSizeHi, iRemainingSizeLo, iRemainingSizeHi, 
					pFileInfo->GetTime(), BoolToStr(pFileInfo->GetFilenameConfirmed()), 
					BoolToStr(pFileInfo->GetPaused()), pFileInfo->GetNZBInfo()->GetID(), xmlNZBNicename,
					xmlNZBNicename, xmlNZBFilename, xmlSubject, xmlFilename, xmlDestDir, xmlCategory,
					pFileInfo->GetNZBInfo()->GetPriority(), pFileInfo->GetActiveDownloads());
				szItemBuf[iItemBufSize-1] = '\0';

				free(xmlNZBFilename);
				free(xmlSubject);
				free(xmlFilename);
				free(xmlDestDir);
				free(xmlCategory);
				free(xmlNZBNicename);

				if (IsJson() && index++ > 0)
				{
					AppendResponse(",\n");
				}
				AppendResponse(szItemBuf);
			}
		}
	}
	free(szItemBuf);

	DownloadQueue::Unlock();
	AppendResponse(IsJson() ? "\n]" : "</data></array>\n");
}

void NzbInfoXmlCommand::AppendNZBInfoFields(NZBInfo* pNZBInfo)
{
	const char* XML_NZB_ITEM_START =
	"<member><name>NZBID</name><value><i4>%i</i4></value></member>\n"
	"<member><name>NZBName</name><value><string>%s</string></value></member>\n"
	"<member><name>NZBNicename</name><value><string>%s</string></value></member>\n"	// deprecated, use "NZBName" instead
	"<member><name>Kind</name><value><string>%s</string></value></member>\n"
	"<member><name>URL</name><value><string>%s</string></value></member>\n"
	"<member><name>NZBFilename</name><value><string>%s</string></value></member>\n"
	"<member><name>DestDir</name><value><string>%s</string></value></member>\n"
	"<member><name>FinalDir</name><value><string>%s</string></value></member>\n"
	"<member><name>Category</name><value><string>%s</string></value></member>\n"
	"<member><name>ParStatus</name><value><string>%s</string></value></member>\n"
	"<member><name>UnpackStatus</name><value><string>%s</string></value></member>\n"
	"<member><name>MoveStatus</name><value><string>%s</string></value></member>\n"
	"<member><name>ScriptStatus</name><value><string>%s</string></value></member>\n"
	"<member><name>DeleteStatus</name><value><string>%s</string></value></member>\n"
	"<member><name>MarkStatus</name><value><string>%s</string></value></member>\n"
	"<member><name>UrlStatus</name><value><string>%s</string></value></member>\n"
	"<member><name>FileSizeLo</name><value><i4>%u</i4></value></member>\n"
	"<member><name>FileSizeHi</name><value><i4>%u</i4></value></member>\n"
	"<member><name>FileSizeMB</name><value><i4>%i</i4></value></member>\n"
	"<member><name>FileCount</name><value><i4>%i</i4></value></member>\n"
	"<member><name>MinPostTime</name><value><i4>%i</i4></value></member>\n"
	"<member><name>MaxPostTime</name><value><i4>%i</i4></value></member>\n"
	"<member><name>TotalArticles</name><value><i4>%i</i4></value></member>\n"
	"<member><name>SuccessArticles</name><value><i4>%i</i4></value></member>\n"
	"<member><name>FailedArticles</name><value><i4>%i</i4></value></member>\n"
	"<member><name>Health</name><value><i4>%i</i4></value></member>\n"
	"<member><name>CriticalHealth</name><value><i4>%i</i4></value></member>\n"
	"<member><name>DupeKey</name><value><string>%s</string></value></member>\n"
	"<member><name>DupeScore</name><value><i4>%i</i4></value></member>\n"
	"<member><name>DupeMode</name><value><string>%s</string></value></member>\n"
	"<member><name>Deleted</name><value><boolean>%s</boolean></value></member>\n"	 // deprecated, use "DeleteStatus" instead
	"<member><name>DownloadedSizeLo</name><value><i4>%u</i4></value></member>\n"
	"<member><name>DownloadedSizeHi</name><value><i4>%u</i4></value></member>\n"
	"<member><name>DownloadedSizeMB</name><value><i4>%i</i4></value></member>\n"
	"<member><name>DownloadTimeSec</name><value><i4>%i</i4></value></member>\n"
	"<member><name>PostTotalTimeSec</name><value><i4>%i</i4></value></member>\n"
	"<member><name>ParTimeSec</name><value><i4>%i</i4></value></member>\n"
	"<member><name>RepairTimeSec</name><value><i4>%i</i4></value></member>\n"
	"<member><name>UnpackTimeSec</name><value><i4>%i</i4></value></member>\n"
	"<member><name>Parameters</name><value><array><data>\n";

	const char* XML_NZB_ITEM_SCRIPT_START =
	"</data></array></value></member>\n"
	"<member><name>ScriptStatuses</name><value><array><data>\n";
	
	const char* XML_NZB_ITEM_STATS_START =
	"</data></array></value></member>\n"
	"<member><name>ServerStats</name><value><array><data>\n";
	
	const char* XML_NZB_ITEM_END =
	"</data></array></value></member>\n";
	
	const char* JSON_NZB_ITEM_START =
	"\"NZBID\" : %i,\n"
	"\"NZBName\" : \"%s\",\n"
	"\"NZBNicename\" : \"%s\",\n"		// deprecated, use NZBName instead
	"\"Kind\" : \"%s\",\n"
	"\"URL\" : \"%s\",\n"
	"\"NZBFilename\" : \"%s\",\n"
	"\"DestDir\" : \"%s\",\n"
	"\"FinalDir\" : \"%s\",\n"
	"\"Category\" : \"%s\",\n"
	"\"ParStatus\" : \"%s\",\n"
	"\"UnpackStatus\" : \"%s\",\n"
	"\"MoveStatus\" : \"%s\",\n"
	"\"ScriptStatus\" : \"%s\",\n"
	"\"DeleteStatus\" : \"%s\",\n"
	"\"MarkStatus\" : \"%s\",\n"
	"\"UrlStatus\" : \"%s\",\n"
	"\"FileSizeLo\" : %u,\n"
	"\"FileSizeHi\" : %u,\n"
	"\"FileSizeMB\" : %i,\n"
	"\"FileCount\" : %i,\n"
	"\"MinPostTime\" : %i,\n"
	"\"MaxPostTime\" : %i,\n"
	"\"TotalArticles\" : %i,\n"
	"\"SuccessArticles\" : %i,\n"
	"\"FailedArticles\" : %i,\n"
	"\"Health\" : %i,\n"
	"\"CriticalHealth\" : %i,\n"
	"\"DupeKey\" : \"%s\",\n"
	"\"DupeScore\" : %i,\n"
	"\"DupeMode\" : \"%s\",\n"
	"\"Deleted\" : %s,\n"			  // deprecated, use "DeleteStatus" instead
	"\"DownloadedSizeLo\" : %u,\n"
	"\"DownloadedSizeHi\" : %u,\n"
	"\"DownloadedSizeMB\" : %i,\n"
	"\"DownloadTimeSec\" : %i,\n"
	"\"PostTotalTimeSec\" : %i,\n"
	"\"ParTimeSec\" : %i,\n"
	"\"RepairTimeSec\" : %i,\n"
	"\"UnpackTimeSec\" : %i,\n"
	"\"Parameters\" : [\n";

	const char* JSON_NZB_ITEM_SCRIPT_START =
	"],\n"
	"\"ScriptStatuses\" : [\n";
	
	const char* JSON_NZB_ITEM_STATS_START =
	"],\n"
	"\"ServerStats\" : [\n";
	
	const char* JSON_NZB_ITEM_END =
	"],\n";
	
	const char* XML_PARAMETER_ITEM =
	"<value><struct>\n"
	"<member><name>Name</name><value><string>%s</string></value></member>\n"
	"<member><name>Value</name><value><string>%s</string></value></member>\n"
	"</struct></value>\n";
	
	const char* JSON_PARAMETER_ITEM =
	"{\n"
	"\"Name\" : \"%s\",\n"
	"\"Value\" : \"%s\"\n"
	"}";
	
	const char* XML_SCRIPT_ITEM =
	"<value><struct>\n"
	"<member><name>Name</name><value><string>%s</string></value></member>\n"
	"<member><name>Status</name><value><string>%s</string></value></member>\n"
	"</struct></value>\n";
	
	const char* JSON_SCRIPT_ITEM =
	"{\n"
	"\"Name\" : \"%s\",\n"
	"\"Status\" : \"%s\"\n"
	"}";
	
	const char* XML_STAT_ITEM =
	"<value><struct>\n"
	"<member><name>ServerID</name><value><i4>%i</i4></value></member>\n"
	"<member><name>SuccessArticles</name><value><i4>%i</i4></value></member>\n"
	"<member><name>FailedArticles</name><value><i4>%i</i4></value></member>\n"
	"</struct></value>\n";
	
	const char* JSON_STAT_ITEM =
	"{\n"
	"\"ServerID\" : %i,\n"
	"\"SuccessArticles\" : %i,\n"
	"\"FailedArticles\" : %i\n"
	"}";
	
    const char* szKindName[] = { "NZB", "URL" };
    const char* szParStatusName[] = { "NONE", "NONE", "FAILURE", "SUCCESS", "REPAIR_POSSIBLE", "MANUAL" };
    const char* szUnpackStatusName[] = { "NONE", "NONE", "FAILURE", "SUCCESS", "SPACE", "PASSWORD" };
    const char* szMoveStatusName[] = { "NONE", "FAILURE", "SUCCESS" };
    const char* szScriptStatusName[] = { "NONE", "FAILURE", "SUCCESS" };
    const char* szDeleteStatusName[] = { "NONE", "MANUAL", "HEALTH", "DUPE", "BAD" };
    const char* szMarkStatusName[] = { "NONE", "BAD", "GOOD" };
	const char* szUrlStatusName[] = { "NONE", "UNKNOWN", "SUCCESS", "FAILURE", "UNKNOWN", "SCAN_SKIPPED", "SCAN_FAILURE" };
    const char* szDupeModeName[] = { "SCORE", "ALL", "FORCE" };
	
	int iItemBufSize = 10240;
	char* szItemBuf = (char*)malloc(iItemBufSize);
	
	unsigned long iFileSizeHi, iFileSizeLo, iFileSizeMB;
	Util::SplitInt64(pNZBInfo->GetSize(), &iFileSizeHi, &iFileSizeLo);
	iFileSizeMB = (int)(pNZBInfo->GetSize() / 1024 / 1024);

	unsigned long iDownloadedSizeHi, iDownloadedSizeLo, iDownloadedSizeMB;
	Util::SplitInt64(pNZBInfo->GetDownloadedSize(), &iDownloadedSizeHi, &iDownloadedSizeLo);
	iDownloadedSizeMB = (int)(pNZBInfo->GetDownloadedSize() / 1024 / 1024);

	char* xmlURL = EncodeStr(pNZBInfo->GetURL());
	char* xmlNZBFilename = EncodeStr(pNZBInfo->GetFilename());
	char* xmlNZBNicename = EncodeStr(pNZBInfo->GetName());
	char* xmlDestDir = EncodeStr(pNZBInfo->GetDestDir());
	char* xmlFinalDir = EncodeStr(pNZBInfo->GetFinalDir());
	char* xmlCategory = EncodeStr(pNZBInfo->GetCategory());
	char* xmlDupeKey = EncodeStr(pNZBInfo->GetDupeKey());
	
	snprintf(szItemBuf, iItemBufSize, IsJson() ? JSON_NZB_ITEM_START : XML_NZB_ITEM_START,
			 pNZBInfo->GetID(), xmlNZBNicename, xmlNZBNicename, szKindName[pNZBInfo->GetKind()],
			 xmlURL, xmlNZBFilename, xmlDestDir, xmlFinalDir, xmlCategory,
			 szParStatusName[pNZBInfo->GetParStatus()], szUnpackStatusName[pNZBInfo->GetUnpackStatus()],
			 szMoveStatusName[pNZBInfo->GetMoveStatus()],
			 szScriptStatusName[pNZBInfo->GetScriptStatuses()->CalcTotalStatus()],
			 szDeleteStatusName[pNZBInfo->GetDeleteStatus()], szMarkStatusName[pNZBInfo->GetMarkStatus()],
			 szUrlStatusName[pNZBInfo->GetUrlStatus()],
			 iFileSizeLo, iFileSizeHi, iFileSizeMB, pNZBInfo->GetFileCount(),
			 pNZBInfo->GetMinTime(), pNZBInfo->GetMaxTime(),
			 pNZBInfo->GetTotalArticles(), pNZBInfo->GetCurrentSuccessArticles(), pNZBInfo->GetCurrentFailedArticles(),
			 pNZBInfo->CalcHealth(), pNZBInfo->CalcCriticalHealth(false),
			 xmlDupeKey, pNZBInfo->GetDupeScore(), szDupeModeName[pNZBInfo->GetDupeMode()],
			 BoolToStr(pNZBInfo->GetDeleteStatus() != NZBInfo::dsNone),
			 iDownloadedSizeLo, iDownloadedSizeHi, iDownloadedSizeMB, pNZBInfo->GetDownloadSec(), 
			 pNZBInfo->GetPostInfo() && pNZBInfo->GetPostInfo()->GetStartTime() ? time(NULL) - pNZBInfo->GetPostInfo()->GetStartTime() : pNZBInfo->GetPostTotalSec(),
			 pNZBInfo->GetParSec(), pNZBInfo->GetRepairSec(), pNZBInfo->GetUnpackSec());

	free(xmlURL);
	free(xmlNZBNicename);
	free(xmlNZBFilename);
	free(xmlCategory);
	free(xmlDestDir);
	free(xmlFinalDir);
	free(xmlDupeKey);
		
	szItemBuf[iItemBufSize-1] = '\0';

	AppendResponse(szItemBuf);
	
	// Post-processing parameters
	int iParamIndex = 0;
	for (NZBParameterList::iterator it = pNZBInfo->GetParameters()->begin(); it != pNZBInfo->GetParameters()->end(); it++)
	{
		NZBParameter* pParameter = *it;
		
		char* xmlName = EncodeStr(pParameter->GetName());
		char* xmlValue = EncodeStr(pParameter->GetValue());
		
		snprintf(szItemBuf, iItemBufSize, IsJson() ? JSON_PARAMETER_ITEM : XML_PARAMETER_ITEM, xmlName, xmlValue);
		szItemBuf[iItemBufSize-1] = '\0';
		
		free(xmlName);
		free(xmlValue);
		
		if (IsJson() && iParamIndex++ > 0)
		{
			AppendResponse(",\n");
		}
		AppendResponse(szItemBuf);
	}
	
	AppendResponse(IsJson() ? JSON_NZB_ITEM_SCRIPT_START : XML_NZB_ITEM_SCRIPT_START);
	
	// Script statuses
	int iScriptIndex = 0;
	for (ScriptStatusList::iterator it = pNZBInfo->GetScriptStatuses()->begin(); it != pNZBInfo->GetScriptStatuses()->end(); it++)
	{
		ScriptStatus* pScriptStatus = *it;
		
		char* xmlName = EncodeStr(pScriptStatus->GetName());
		char* xmlStatus = EncodeStr(szScriptStatusName[pScriptStatus->GetStatus()]);
		
		snprintf(szItemBuf, iItemBufSize, IsJson() ? JSON_SCRIPT_ITEM : XML_SCRIPT_ITEM, xmlName, xmlStatus);
		szItemBuf[iItemBufSize-1] = '\0';
		
		free(xmlName);
		free(xmlStatus);
		
		if (IsJson() && iScriptIndex++ > 0)
		{
			AppendResponse(",\n");
		}
		AppendResponse(szItemBuf);
	}
	
	AppendResponse(IsJson() ? JSON_NZB_ITEM_STATS_START : XML_NZB_ITEM_STATS_START);
	
	// Server stats
	int iStatIndex = 0;
	for (ServerStatList::iterator it = pNZBInfo->GetCurrentServerStats()->begin(); it != pNZBInfo->GetCurrentServerStats()->end(); it++)
	{
		ServerStat* pServerStat = *it;
		
		snprintf(szItemBuf, iItemBufSize, IsJson() ? JSON_STAT_ITEM : XML_STAT_ITEM,
				 pServerStat->GetServerID(), pServerStat->GetSuccessArticles(), pServerStat->GetFailedArticles());
		szItemBuf[iItemBufSize-1] = '\0';
		
		if (IsJson() && iStatIndex++ > 0)
		{
			AppendResponse(",\n");
		}
		AppendResponse(szItemBuf);
	}
	
	AppendResponse(IsJson() ? JSON_NZB_ITEM_END : XML_NZB_ITEM_END);

	free(szItemBuf);
}

void NzbInfoXmlCommand::AppendPostInfoFields(PostInfo* pPostInfo, int iLogEntries, bool bPostQueue)
{
	const char* XML_GROUPQUEUE_ITEM_START = 
		"<member><name>PostInfoText</name><value><string>%s</string></value></member>\n"
		"<member><name>PostStageProgress</name><value><i4>%i</i4></value></member>\n"
		"<member><name>PostStageTimeSec</name><value><i4>%i</i4></value></member>\n";
		// PostTotalTimeSec is printed by method "AppendNZBInfoFields"

	const char* XML_POSTQUEUE_ITEM_START = 
		"<member><name>ProgressLabel</name><value><string>%s</string></value></member>\n"
		"<member><name>StageProgress</name><value><i4>%i</i4></value></member>\n"
		"<member><name>StageTimeSec</name><value><i4>%i</i4></value></member>\n"
		"<member><name>TotalTimeSec</name><value><i4>%i</i4></value></member>\n";

	const char* XML_LOG_START =
		"<member><name>Log</name><value><array><data>\n";

	const char* XML_POSTQUEUE_ITEM_END =
		"</data></array></value></member>\n";
		
	const char* JSON_GROUPQUEUE_ITEM_START =
		"\"PostInfoText\" : \"%s\",\n"
		"\"PostStageProgress\" : %i,\n"
		"\"PostStageTimeSec\" : %i,\n";

	const char* JSON_POSTQUEUE_ITEM_START =
		"\"ProgressLabel\" : \"%s\",\n"
		"\"StageProgress\" : %i,\n"
		"\"StageTimeSec\" : %i,\n"
		"\"TotalTimeSec\" : %i,\n";

	const char* JSON_LOG_START =
		"\"Log\" : [\n";
	
	const char* JSON_POSTQUEUE_ITEM_END =
		"]\n";

	const char* XML_LOG_ITEM =
		"<value><struct>\n"
		"<member><name>ID</name><value><i4>%i</i4></value></member>\n"
		"<member><name>Kind</name><value><string>%s</string></value></member>\n"
		"<member><name>Time</name><value><i4>%i</i4></value></member>\n"
		"<member><name>Text</name><value><string>%s</string></value></member>\n"
		"</struct></value>\n";

	const char* JSON_LOG_ITEM =
		"{\n"
		"\"ID\" : %i,\n"
		"\"Kind\" : \"%s\",\n"
		"\"Time\" : %i,\n"
		"\"Text\" : \"%s\"\n"
		"}";
	
	const char* szMessageType[] = { "INFO", "WARNING", "ERROR", "DEBUG", "DETAIL"};

	time_t tCurTime = time(NULL);
	int iItemBufSize = 10240;
	char* szItemBuf = (char*)malloc(iItemBufSize);
	int index = 0;

	const char* szItemStart = bPostQueue ? IsJson() ? JSON_POSTQUEUE_ITEM_START : XML_POSTQUEUE_ITEM_START :
		IsJson() ? JSON_GROUPQUEUE_ITEM_START : XML_GROUPQUEUE_ITEM_START;

	if (pPostInfo)
	{
		char* xmlProgressLabel = EncodeStr(pPostInfo->GetProgressLabel());

		snprintf(szItemBuf, iItemBufSize,  szItemStart, xmlProgressLabel, pPostInfo->GetStageProgress(),
			pPostInfo->GetStageTime() ? tCurTime - pPostInfo->GetStageTime() : 0,
			pPostInfo->GetStartTime() ? tCurTime - pPostInfo->GetStartTime() : 0);

		free(xmlProgressLabel);
	}
	else
	{
		snprintf(szItemBuf, iItemBufSize,  szItemStart, "NONE", "", 0, 0, 0, 0);
	}

	szItemBuf[iItemBufSize-1] = '\0';

	if (IsJson() && index++ > 0)
	{
		AppendResponse(",\n");
	}
	AppendResponse(szItemBuf);
	
	AppendResponse(IsJson() ? JSON_LOG_START : XML_LOG_START);

	if (iLogEntries > 0 && pPostInfo)
	{
		PostInfo::Messages* pMessages = pPostInfo->LockMessages();
		if (!pMessages->empty())
		{
			if (iLogEntries > (int)pMessages->size())
			{
				iLogEntries = pMessages->size();
			}
			int iStart = pMessages->size() - iLogEntries;

			int index = 0;
			for (unsigned int i = (unsigned int)iStart; i < pMessages->size(); i++)
			{
				Message* pMessage = (*pMessages)[i];
				char* xmltext = EncodeStr(pMessage->GetText());
				snprintf(szItemBuf, iItemBufSize, IsJson() ? JSON_LOG_ITEM : XML_LOG_ITEM,
					pMessage->GetID(), szMessageType[pMessage->GetKind()], pMessage->GetTime(), xmltext);
				szItemBuf[iItemBufSize-1] = '\0';
				free(xmltext);

				if (IsJson() && index++ > 0)
				{
					AppendResponse(",\n");
				}
				AppendResponse(szItemBuf);
			}
		}
		pPostInfo->UnlockMessages();
	}

	AppendResponse(IsJson() ? JSON_POSTQUEUE_ITEM_END : XML_POSTQUEUE_ITEM_END);

	free(szItemBuf);
}

// struct[] listgroups(int NumberOfLogEntries)
void ListGroupsXmlCommand::Execute()
{
	int iNrEntries = 0;
	NextParamAsInt(&iNrEntries);

	AppendResponse(IsJson() ? "[\n" : "<array><data>\n");

	const char* XML_LIST_ITEM_START = 
		"<value><struct>\n"
		"<member><name>FirstID</name><value><i4>%i</i4></value></member>\n"				// deprecated, use "NZBID" instead
		"<member><name>LastID</name><value><i4>%i</i4></value></member>\n"				// deprecated, use "NZBID" instead
		"<member><name>RemainingSizeLo</name><value><i4>%u</i4></value></member>\n"
		"<member><name>RemainingSizeHi</name><value><i4>%u</i4></value></member>\n"
		"<member><name>RemainingSizeMB</name><value><i4>%i</i4></value></member>\n"
		"<member><name>PausedSizeLo</name><value><i4>%u</i4></value></member>\n"
		"<member><name>PausedSizeHi</name><value><i4>%u</i4></value></member>\n"
		"<member><name>PausedSizeMB</name><value><i4>%i</i4></value></member>\n"
		"<member><name>RemainingFileCount</name><value><i4>%i</i4></value></member>\n"
		"<member><name>RemainingParCount</name><value><i4>%i</i4></value></member>\n"
		"<member><name>MinPriority</name><value><i4>%i</i4></value></member>\n"
		"<member><name>MaxPriority</name><value><i4>%i</i4></value></member>\n"
		"<member><name>ActiveDownloads</name><value><i4>%i</i4></value></member>\n"
		"<member><name>Status</name><value><string>%s</string></value></member>\n";

	const char* XML_LIST_ITEM_END = 
		"</struct></value>\n";

	const char* JSON_LIST_ITEM_START = 
		"{\n"
		"\"FirstID\" : %i,\n"					// deprecated, use "NZBID" instead
		"\"LastID\" : %i,\n"					// deprecated, use "NZBID" instead
		"\"RemainingSizeLo\" : %u,\n"
		"\"RemainingSizeHi\" : %u,\n"
		"\"RemainingSizeMB\" : %i,\n"
		"\"PausedSizeLo\" : %u,\n"
		"\"PausedSizeHi\" : %u,\n"
		"\"PausedSizeMB\" : %i,\n"
		"\"RemainingFileCount\" : %i,\n"
		"\"RemainingParCount\" : %i,\n"
		"\"MinPriority\" : %i,\n"
		"\"MaxPriority\" : %i,\n"
		"\"ActiveDownloads\" : %i,\n"
		"\"Status\" : \"%s\",\n";
	
	const char* JSON_LIST_ITEM_END =
		"}";

	int iItemBufSize = 10240;
	char* szItemBuf = (char*)malloc(iItemBufSize);
	int index = 0;

	DownloadQueue* pDownloadQueue = DownloadQueue::Lock();

	for (NZBList::iterator it = pDownloadQueue->GetQueue()->begin(); it != pDownloadQueue->GetQueue()->end(); it++)
	{
		NZBInfo* pNZBInfo = *it;

		unsigned long iRemainingSizeLo, iRemainingSizeHi, iRemainingSizeMB;
		unsigned long iPausedSizeLo, iPausedSizeHi, iPausedSizeMB;
		Util::SplitInt64(pNZBInfo->GetRemainingSize(), &iRemainingSizeHi, &iRemainingSizeLo);
		iRemainingSizeMB = (int)(pNZBInfo->GetRemainingSize() / 1024 / 1024);
		Util::SplitInt64(pNZBInfo->GetPausedSize(), &iPausedSizeHi, &iPausedSizeLo);
		iPausedSizeMB = (int)(pNZBInfo->GetPausedSize() / 1024 / 1024);
		const char* szStatus = DetectStatus(pNZBInfo);

		snprintf(szItemBuf, iItemBufSize, IsJson() ? JSON_LIST_ITEM_START : XML_LIST_ITEM_START,
			pNZBInfo->GetID(), pNZBInfo->GetID(), iRemainingSizeLo, iRemainingSizeHi, iRemainingSizeMB,
			iPausedSizeLo, iPausedSizeHi, iPausedSizeMB, (int)pNZBInfo->GetFileList()->size(),
			pNZBInfo->GetRemainingParCount(), pNZBInfo->GetPriority(), pNZBInfo->GetPriority(),
			pNZBInfo->GetActiveDownloads(), szStatus);
		szItemBuf[iItemBufSize-1] = '\0';

		if (IsJson() && index++ > 0)
		{
			AppendResponse(",\n");
		}
		AppendResponse(szItemBuf);
		
		AppendNZBInfoFields(pNZBInfo);
		AppendPostInfoFields(pNZBInfo->GetPostInfo(), iNrEntries, false);

		AppendResponse(IsJson() ? JSON_LIST_ITEM_END : XML_LIST_ITEM_END);
	}

	DownloadQueue::Unlock();

	free(szItemBuf);

	AppendResponse(IsJson() ? "\n]" : "</data></array>\n");
}

const char* ListGroupsXmlCommand::DetectStatus(NZBInfo* pNZBInfo)
{
    const char* szPostStageName[] = { "PP_QUEUED", "LOADING_PARS", "VERIFYING_SOURCES", "REPAIRING", "VERIFYING_REPAIRED", "RENAMING", "UNPACKING", "MOVING", "EXECUTING_SCRIPT", "PP_FINISHED" };

	const char* szStatus = NULL;

	if (pNZBInfo->GetPostInfo())
	{
		szStatus = szPostStageName[pNZBInfo->GetPostInfo()->GetStage()];
	}
	else if (pNZBInfo->GetActiveDownloads() > 0)
	{
		szStatus = pNZBInfo->GetKind() == NZBInfo::nkUrl ? "FETCHING" : "DOWNLOADING";
	}
	else if ((pNZBInfo->GetPausedSize() > 0) && (pNZBInfo->GetRemainingSize() == pNZBInfo->GetPausedSize()))
	{
		szStatus = "PAUSED";
	}
	else
	{
		szStatus = "QUEUED";
	}

	return szStatus;
}

typedef struct 
{
	int				iActionID;
	const char*		szActionName;
} EditCommandEntry;

EditCommandEntry EditCommandNameMap[] = { 
	{ DownloadQueue::eaFileMoveOffset, "FileMoveOffset" },
	{ DownloadQueue::eaFileMoveTop, "FileMoveTop" },
	{ DownloadQueue::eaFileMoveBottom, "FileMoveBottom" },
	{ DownloadQueue::eaFilePause, "FilePause" },
	{ DownloadQueue::eaFileResume, "FileResume" },
	{ DownloadQueue::eaFileDelete, "FileDelete" },
	{ DownloadQueue::eaFilePauseAllPars, "FilePauseAllPars" },
	{ DownloadQueue::eaFilePauseExtraPars, "FilePauseExtraPars" },
	{ DownloadQueue::eaFileReorder, "FileReorder" },
	{ DownloadQueue::eaFileSplit, "FileSplit" },
	{ DownloadQueue::eaGroupMoveOffset, "GroupMoveOffset" },
	{ DownloadQueue::eaGroupMoveTop, "GroupMoveTop" },
	{ DownloadQueue::eaGroupMoveBottom, "GroupMoveBottom" },
	{ DownloadQueue::eaGroupPause, "GroupPause" },
	{ DownloadQueue::eaGroupResume, "GroupResume" },
	{ DownloadQueue::eaGroupDelete, "GroupDelete" },
	{ DownloadQueue::eaGroupDupeDelete, "GroupDupeDelete" },
	{ DownloadQueue::eaGroupFinalDelete, "GroupFinalDelete" },
	{ DownloadQueue::eaGroupPauseAllPars, "GroupPauseAllPars" },
	{ DownloadQueue::eaGroupPauseExtraPars, "GroupPauseExtraPars" },
	{ DownloadQueue::eaGroupSetPriority, "GroupSetPriority" },
	{ DownloadQueue::eaGroupSetCategory, "GroupSetCategory" },
	{ DownloadQueue::eaGroupApplyCategory, "GroupApplyCategory" },
	{ DownloadQueue::eaGroupMerge, "GroupMerge" },
	{ DownloadQueue::eaGroupSetParameter, "GroupSetParameter" },
	{ DownloadQueue::eaGroupSetName, "GroupSetName" },
	{ DownloadQueue::eaGroupSetDupeKey, "GroupSetDupeKey" },
	{ DownloadQueue::eaGroupSetDupeScore, "GroupSetDupeScore" },
	{ DownloadQueue::eaGroupSetDupeMode, "GroupSetDupeMode" },
	{ DownloadQueue::eaPostDelete, "PostDelete" },
	{ DownloadQueue::eaHistoryDelete, "HistoryDelete" },
	{ DownloadQueue::eaHistoryFinalDelete, "HistoryFinalDelete" },
	{ DownloadQueue::eaHistoryReturn, "HistoryReturn" },
	{ DownloadQueue::eaHistoryProcess, "HistoryProcess" },		
	{ DownloadQueue::eaHistoryRedownload, "HistoryRedownload" },
	{ DownloadQueue::eaHistorySetParameter, "HistorySetParameter" },
	{ DownloadQueue::eaHistorySetDupeKey, "HistorySetDupeKey" },
	{ DownloadQueue::eaHistorySetDupeScore, "HistorySetDupeScore" },
	{ DownloadQueue::eaHistorySetDupeMode, "HistorySetDupeMode" },
	{ DownloadQueue::eaHistorySetDupeBackup, "HistorySetDupeBackup" },
	{ DownloadQueue::eaHistoryMarkBad, "HistoryMarkBad" },
	{ DownloadQueue::eaHistoryMarkGood, "HistoryMarkGood" },
	{ 0, NULL }
};

void EditQueueXmlCommand::Execute()
{
	if (!CheckSafeMethod())
	{
		return;
	}

	char* szEditCommand;
	if (!NextParamAsStr(&szEditCommand))
	{
		BuildErrorResponse(2, "Invalid parameter");
		return;
	}
	debug("EditCommand=%s", szEditCommand);

	int iAction = -1;
	for (int i = 0; const char* szName = EditCommandNameMap[i].szActionName; i++)
	{
		if (!strcasecmp(szEditCommand, szName))
		{
			iAction = EditCommandNameMap[i].iActionID;
			break;
		}
	}

	if (iAction == -1)
	{
		BuildErrorResponse(3, "Invalid action");
		return;
	}

	int iOffset = 0;
	if (!NextParamAsInt(&iOffset))
	{
		BuildErrorResponse(2, "Invalid parameter");
		return;
	}

	char* szEditText;
	if (!NextParamAsStr(&szEditText))
	{
		BuildErrorResponse(2, "Invalid parameter");
		return;
	}
	debug("EditText=%s", szEditText);

	DecodeStr(szEditText);

	IDList cIDList;
	int iID = 0;
	while (NextParamAsInt(&iID))
	{
		cIDList.push_back(iID);
	}

	DownloadQueue* pDownloadQueue = DownloadQueue::Lock();
	bool bOK = pDownloadQueue->EditList(&cIDList, NULL, DownloadQueue::mmID, (DownloadQueue::EEditAction)iAction, iOffset, szEditText);
	DownloadQueue::Unlock();

	BuildBoolResponse(bOK);
}

// v13 (new param order and new result type):
//   int append(string NZBFilename, string NZBContent, string Category, int Priority, bool AddToTop, bool AddPaused, string DupeKey, int DupeScore, string DupeMode)
// v12 (backward compatible, some params are optional):
//   bool append(string NZBFilename, string Category, int Priority, bool AddToTop, string Content, bool AddPaused, string DupeKey, int DupeScore, string DupeMode)
void DownloadXmlCommand::Execute()
{
	if (!CheckSafeMethod())
	{
		return;
	}

	bool bV13 = true;

	char* szNZBFilename;
	if (!NextParamAsStr(&szNZBFilename))
	{
		BuildErrorResponse(2, "Invalid parameter (NZBFileName)");
		return;
	}

	char* szNZBContent;
	if (!NextParamAsStr(&szNZBContent))
	{
		BuildErrorResponse(2, "Invalid parameter (NZBContent)");
		return;
	}

	char* szCategory;
	if (!NextParamAsStr(&szCategory))
	{
		bV13 = false;
		szCategory = szNZBContent;
	}

	DecodeStr(szNZBFilename);
	DecodeStr(szCategory);

	debug("FileName=%s", szNZBFilename);

	// For backward compatibility with 0.8 parameter "Priority" is optional (error checking omitted)
	int iPriority = 0;
	NextParamAsInt(&iPriority);

	bool bAddTop;
	if (!NextParamAsBool(&bAddTop))
	{
		BuildErrorResponse(2, "Invalid parameter (AddTop)");
		return;
	}

	if (!bV13 && !NextParamAsStr(&szNZBContent))
	{
		BuildErrorResponse(2, "Invalid parameter (FileContent)");
		return;
	}
	DecodeStr(szNZBContent);

	bool bAddPaused = false;
	char* szDupeKey = NULL;
	int iDupeScore = 0;
	EDupeMode eDupeMode = dmScore;
	if (NextParamAsBool(&bAddPaused))
	{
		if (!NextParamAsStr(&szDupeKey))
		{
			BuildErrorResponse(2, "Invalid parameter (DupeKey)");
			return;
		}
		DecodeStr(szDupeKey);
		if (!NextParamAsInt(&iDupeScore))
		{
			BuildErrorResponse(2, "Invalid parameter (DupeScore)");
			return;
		}
		char* szDupeMode = NULL;
		if (!NextParamAsStr(&szDupeMode) ||
			(strcasecmp(szDupeMode, "score") && strcasecmp(szDupeMode, "all") && strcasecmp(szDupeMode, "force")))
		{
			BuildErrorResponse(2, "Invalid parameter (DupeMode)");
			return;
		}
		eDupeMode = !strcasecmp(szDupeMode, "all") ? dmAll :
			!strcasecmp(szDupeMode, "force") ? dmForce : dmScore;
	}
	else if (bV13)
	{
		BuildErrorResponse(2, "Invalid parameter (AddPaused)");
		return;
	}

	if (!strncasecmp(szNZBContent, "http://", 6) || !strncasecmp(szNZBContent, "https://", 7))
	{
		// add url
		NZBInfo* pNZBInfo = new NZBInfo();
		pNZBInfo->SetKind(NZBInfo::nkUrl);
		pNZBInfo->SetURL(szNZBContent);
		pNZBInfo->SetFilename(szNZBFilename);
		pNZBInfo->SetCategory(szCategory);
		pNZBInfo->SetPriority(iPriority);
		pNZBInfo->SetAddUrlPaused(bAddPaused);
		pNZBInfo->SetDupeKey(szDupeKey ? szDupeKey : "");
		pNZBInfo->SetDupeScore(iDupeScore);
		pNZBInfo->SetDupeMode(eDupeMode);
		int iNZBID = pNZBInfo->GetID();

		char szNicename[1024];
		pNZBInfo->MakeNiceUrlName(szNZBContent, szNZBFilename, szNicename, sizeof(szNicename));
		info("Queue %s", szNicename);

		DownloadQueue* pDownloadQueue = DownloadQueue::Lock();
		pDownloadQueue->GetQueue()->Add(pNZBInfo, bAddTop);
		pDownloadQueue->Save();
		DownloadQueue::Unlock();

		if (bV13)
		{
			BuildIntResponse(iNZBID);
		}
		else
		{
			BuildBoolResponse(true);
		}
	}
	else
	{
		// add file content
		int iLen = WebUtil::DecodeBase64(szNZBContent, 0, szNZBContent);
		szNZBContent[iLen] = '\0';
		//debug("FileContent=%s", szFileContent);

		int iNZBID = 0;
		bool bOK = g_pScanner->AddExternalFile(szNZBFilename, szCategory, iPriority,
			szDupeKey, iDupeScore, eDupeMode, NULL, bAddTop, bAddPaused, NULL,
			NULL, szNZBContent, iLen, &iNZBID) != Scanner::asFailed;

		if (bV13)
		{
			BuildIntResponse(bOK ? iNZBID : -1);
		}
		else
		{
			BuildBoolResponse(bOK);
		}
	}
}

// deprecated
void PostQueueXmlCommand::Execute()
{
	int iNrEntries = 0;
	NextParamAsInt(&iNrEntries);

	AppendResponse(IsJson() ? "[\n" : "<array><data>\n");

	const char* XML_POSTQUEUE_ITEM_START = 
		"<value><struct>\n"
		"<member><name>ID</name><value><i4>%i</i4></value></member>\n"
		"<member><name>InfoName</name><value><string>%s</string></value></member>\n"
		"<member><name>ParFilename</name><value><string></string></value></member>\n"		// deprecated, always empty
		"<member><name>Stage</name><value><string>%s</string></value></member>\n"
		"<member><name>FileProgress</name><value><i4>%i</i4></value></member>\n";

	const char* XML_POSTQUEUE_ITEM_END =
		"</struct></value>\n";
		
	const char* JSON_POSTQUEUE_ITEM_START =
		"{\n"
		"\"ID\" : %i,\n"
		"\"InfoName\" : \"%s\",\n"
		"\"ParFilename\" : \"\",\n"		// deprecated, always empty
		"\"Stage\" : \"%s\",\n"
		"\"FileProgress\" : %i,\n";

	const char* JSON_POSTQUEUE_ITEM_END =
		"}";

    const char* szPostStageName[] = { "QUEUED", "LOADING_PARS", "VERIFYING_SOURCES", "REPAIRING", "VERIFYING_REPAIRED", "RENAMING", "UNPACKING", "MOVING", "EXECUTING_SCRIPT", "FINISHED" };

	NZBList* pNZBList = DownloadQueue::Lock()->GetQueue();

	int iItemBufSize = 10240;
	char* szItemBuf = (char*)malloc(iItemBufSize);
	int index = 0;

	for (NZBList::iterator it = pNZBList->begin(); it != pNZBList->end(); it++)
	{
		NZBInfo* pNZBInfo = *it;
		PostInfo* pPostInfo = pNZBInfo->GetPostInfo();
		if (!pPostInfo)
		{
			continue;
		}

		char* xmlInfoName = EncodeStr(pPostInfo->GetNZBInfo()->GetName());

		snprintf(szItemBuf, iItemBufSize, IsJson() ? JSON_POSTQUEUE_ITEM_START : XML_POSTQUEUE_ITEM_START,
			pNZBInfo->GetID(), xmlInfoName, szPostStageName[pPostInfo->GetStage()], pPostInfo->GetFileProgress());
		szItemBuf[iItemBufSize-1] = '\0';

		free(xmlInfoName);

		if (IsJson() && index++ > 0)
		{
			AppendResponse(",\n");
		}
		AppendResponse(szItemBuf);

		AppendNZBInfoFields(pPostInfo->GetNZBInfo());
		AppendPostInfoFields(pPostInfo, iNrEntries, true);

		AppendResponse(IsJson() ? JSON_POSTQUEUE_ITEM_END : XML_POSTQUEUE_ITEM_END);
	}
	free(szItemBuf);

	DownloadQueue::Unlock();

	AppendResponse(IsJson() ? "\n]" : "</data></array>\n");
}

void WriteLogXmlCommand::Execute()
{
	if (!CheckSafeMethod())
	{
		return;
	}

	char* szKind;
	char* szText;
	if (!NextParamAsStr(&szKind) || !NextParamAsStr(&szText))
	{
		BuildErrorResponse(2, "Invalid parameter");
		return;
	}

	DecodeStr(szText);

	debug("Kind=%s, Text=%s", szKind, szText);

	if (!strcmp(szKind, "INFO"))
	{
		info(szText);
	}
	else if (!strcmp(szKind, "WARNING"))
	{
		warn(szText);
	}
	else if (!strcmp(szKind, "ERROR"))
	{
		error(szText);
	}
	else if (!strcmp(szKind, "DETAIL"))
	{
		detail(szText);
	}
	else if (!strcmp(szKind, "DEBUG"))
	{
		debug(szText);
	} 
	else
	{
		BuildErrorResponse(3, "Invalid Kind");
		return;
	}

	BuildBoolResponse(true);
}

void ClearLogXmlCommand::Execute()
{
	if (!CheckSafeMethod())
	{
		return;
	}

	g_pLog->Clear();

	BuildBoolResponse(true);
}

void ScanXmlCommand::Execute()
{
	if (!CheckSafeMethod())
	{
		return;
	}

	bool bSyncMode = false;
	// optional parameter "SyncMode"
	NextParamAsBool(&bSyncMode);

	g_pScanner->ScanNZBDir(bSyncMode);
	BuildBoolResponse(true);
}

// struct[] history(bool hidden)
// Parameter "hidden" is optional (new in v12)
void HistoryXmlCommand::Execute()
{
	AppendResponse(IsJson() ? "[\n" : "<array><data>\n");

	const char* XML_HISTORY_ITEM_START =
		"<value><struct>\n"
		"<member><name>ID</name><value><i4>%i</i4></value></member>\n"					// Deprecated, use "NZBID" instead
		"<member><name>Name</name><value><string>%s</string></value></member>\n"
		"<member><name>RemainingFileCount</name><value><i4>%i</i4></value></member>\n"
		"<member><name>HistoryTime</name><value><i4>%i</i4></value></member>\n"
		"<member><name>Status</name><value><string>%s</string></value></member>\n";

	const char* XML_HISTORY_ITEM_LOG_START =
		"<member><name>Log</name><value><array><data>\n";

	const char* XML_HISTORY_ITEM_END =
		"</data></array></value></member>\n"
		"</struct></value>\n";

	const char* JSON_HISTORY_ITEM_START =
		"{\n"
		"\"ID\" : %i,\n"							   // Deprecated, use "NZBID" instead
		"\"Name\" : \"%s\",\n"
		"\"RemainingFileCount\" : %i,\n"
		"\"HistoryTime\" : %i,\n"
		"\"Status\" : \"%s\",\n";
	
	const char* JSON_HISTORY_ITEM_LOG_START =
		"\"Log\" : [\n";

	const char* JSON_HISTORY_ITEM_END = 
		"]\n"
		"}";

	const char* XML_LOG_ITEM =
		"<value><struct>\n"
		"<member><name>ID</name><value><i4>%i</i4></value></member>\n"
		"<member><name>Kind</name><value><string>%s</string></value></member>\n"
		"<member><name>Time</name><value><i4>%i</i4></value></member>\n"
		"<member><name>Text</name><value><string>%s</string></value></member>\n"
		"</struct></value>\n";

	const char* JSON_LOG_ITEM = 
		"{\n"
		"\"ID\" : %i,\n"
		"\"Kind\" : \"%s\",\n"
		"\"Time\" : %i,\n"
		"\"Text\" : \"%s\"\n"
		"}";

	const char* XML_HISTORY_DUP_ITEM =
		"<value><struct>\n"
		"<member><name>ID</name><value><i4>%i</i4></value></member>\n"				// Deprecated, use "NZBID" instead
		"<member><name>NZBID</name><value><i4>%i</i4></value></member>\n"
		"<member><name>Kind</name><value><string>%s</string></value></member>\n"
		"<member><name>Name</name><value><string>%s</string></value></member>\n"
		"<member><name>HistoryTime</name><value><i4>%i</i4></value></member>\n"
		"<member><name>FileSizeLo</name><value><i4>%u</i4></value></member>\n"
		"<member><name>FileSizeHi</name><value><i4>%u</i4></value></member>\n"
		"<member><name>FileSizeMB</name><value><i4>%i</i4></value></member>\n"
		"<member><name>DupeKey</name><value><string>%s</string></value></member>\n"
		"<member><name>DupeScore</name><value><i4>%i</i4></value></member>\n"
		"<member><name>DupeMode</name><value><string>%s</string></value></member>\n"
		"<member><name>DupStatus</name><value><string>%s</string></value></member>\n"
		"<member><name>Status</name><value><string>%s</string></value></member>\n"
		"</struct></value>\n";

	const char* JSON_HISTORY_DUP_ITEM =
		"{\n"
		"\"ID\" : %i,\n"							// Deprecated, use "NZBID" instead
		"\"NZBID\" : %i,\n"
		"\"Kind\" : \"%s\",\n"
		"\"Name\" : \"%s\",\n"
		"\"HistoryTime\" : %i,\n"
		"\"FileSizeLo\" : %i,\n"
		"\"FileSizeHi\" : %i,\n"
		"\"FileSizeMB\" : %i,\n"
		"\"DupeKey\" : \"%s\",\n"
		"\"DupeScore\" : %i,\n"
		"\"DupeMode\" : \"%s\",\n"
		"\"DupStatus\" : \"%s\",\n"
		"\"Status\" : \"%s\",\n";

	const char* szDupStatusName[] = { "UNKNOWN", "SUCCESS", "FAILURE", "DELETED", "DUPE", "BAD", "GOOD" };
	const char* szMessageType[] = { "INFO", "WARNING", "ERROR", "DEBUG", "DETAIL"};
    const char* szDupeModeName[] = { "SCORE", "ALL", "FORCE" };

	bool bDup = false;
	NextParamAsBool(&bDup);

	DownloadQueue* pDownloadQueue = DownloadQueue::Lock();

	int iItemBufSize = 10240;
	char* szItemBuf = (char*)malloc(iItemBufSize);
	int index = 0;

	for (HistoryList::iterator it = pDownloadQueue->GetHistory()->begin(); it != pDownloadQueue->GetHistory()->end(); it++)
	{
		HistoryInfo* pHistoryInfo = *it;

		if (pHistoryInfo->GetKind() == HistoryInfo::hkDup && !bDup)
		{
			continue;
		}

		NZBInfo* pNZBInfo = NULL;
		char szNicename[1024];
		pHistoryInfo->GetName(szNicename, sizeof(szNicename));

		char *xmlNicename = EncodeStr(szNicename);
		const char* szStatus = DetectStatus(pHistoryInfo);

		if (pHistoryInfo->GetKind() == HistoryInfo::hkNzb ||
			pHistoryInfo->GetKind() == HistoryInfo::hkUrl)
		{
			pNZBInfo = pHistoryInfo->GetNZBInfo();

			snprintf(szItemBuf, iItemBufSize, IsJson() ? JSON_HISTORY_ITEM_START : XML_HISTORY_ITEM_START,
				pHistoryInfo->GetID(), xmlNicename, pNZBInfo->GetParkedFileCount(),
				pHistoryInfo->GetTime(), szStatus);
		}
		else if (pHistoryInfo->GetKind() == HistoryInfo::hkDup)
		{
			DupInfo* pDupInfo = pHistoryInfo->GetDupInfo();

			unsigned long iFileSizeHi, iFileSizeLo, iFileSizeMB;
			Util::SplitInt64(pDupInfo->GetSize(), &iFileSizeHi, &iFileSizeLo);
			iFileSizeMB = (int)(pDupInfo->GetSize() / 1024 / 1024);

			char* xmlDupeKey = EncodeStr(pDupInfo->GetDupeKey());

			snprintf(szItemBuf, iItemBufSize, IsJson() ? JSON_HISTORY_DUP_ITEM : XML_HISTORY_DUP_ITEM,
				pHistoryInfo->GetID(), pHistoryInfo->GetID(), "DUP", xmlNicename, pHistoryInfo->GetTime(),
				iFileSizeLo, iFileSizeHi, iFileSizeMB, xmlDupeKey, pDupInfo->GetDupeScore(),
				szDupeModeName[pDupInfo->GetDupeMode()], szDupStatusName[pDupInfo->GetStatus()],
				szStatus);

			free(xmlDupeKey);
		}

		szItemBuf[iItemBufSize-1] = '\0';

		free(xmlNicename);

		if (IsJson() && index++ > 0)
		{
			AppendResponse(",\n");
		}
		AppendResponse(szItemBuf);

		if (pNZBInfo)
		{
			AppendNZBInfoFields(pNZBInfo);
		}
		
		AppendResponse(IsJson() ? JSON_HISTORY_ITEM_LOG_START : XML_HISTORY_ITEM_LOG_START);

		if (pNZBInfo)
		{
			// Log-Messages
			NZBInfo::Messages* pMessages = pNZBInfo->LockMessages();
			if (!pMessages->empty())
			{
				int iLogIndex = 0;
				for (NZBInfo::Messages::iterator it = pMessages->begin(); it != pMessages->end(); it++)
				{
					Message* pMessage = *it;
					char* xmltext = EncodeStr(pMessage->GetText());
					snprintf(szItemBuf, iItemBufSize, IsJson() ? JSON_LOG_ITEM : XML_LOG_ITEM,
						pMessage->GetID(), szMessageType[pMessage->GetKind()], pMessage->GetTime(), xmltext);
					szItemBuf[iItemBufSize-1] = '\0';
					free(xmltext);

					if (IsJson() && iLogIndex++ > 0)
					{
						AppendResponse(",\n");
					}
					AppendResponse(szItemBuf);
				}
			}
			pNZBInfo->UnlockMessages();
		}

		AppendResponse(IsJson() ? JSON_HISTORY_ITEM_END : XML_HISTORY_ITEM_END);
	}
	free(szItemBuf);

	AppendResponse(IsJson() ? "\n]" : "</data></array>\n");

	DownloadQueue::Unlock();
}

const char* HistoryXmlCommand::DetectStatus(HistoryInfo* pHistoryInfo)
{
	const char* szStatus = "FAILURE/INTERNAL_ERROR";

	if (pHistoryInfo->GetKind() == HistoryInfo::hkNzb || pHistoryInfo->GetKind() == HistoryInfo::hkUrl)
	{
		NZBInfo* pNZBInfo = pHistoryInfo->GetNZBInfo();
		szStatus = pNZBInfo->MakeTextStatus(false);
	}
	else if (pHistoryInfo->GetKind() == HistoryInfo::hkDup)
	{
		DupInfo* pDupInfo = pHistoryInfo->GetDupInfo();
		const char* szDupStatusName[] = { "FAILURE/INTERNAL_ERROR", "SUCCESS/HIDDEN", "FAILURE/HIDDEN",
			"DELETED/MANUAL", "DELETED/DUPE", "FAILURE/BAD", "SUCCESS/GOOD" };
		szStatus = szDupStatusName[pDupInfo->GetStatus()];
	}

	return szStatus;
}

// Deprecated in v13
void UrlQueueXmlCommand::Execute()
{
	AppendResponse(IsJson() ? "[\n" : "<array><data>\n");

	const char* XML_URLQUEUE_ITEM = 
		"<value><struct>\n"
		"<member><name>ID</name><value><i4>%i</i4></value></member>\n"
		"<member><name>NZBFilename</name><value><string>%s</string></value></member>\n"
		"<member><name>URL</name><value><string>%s</string></value></member>\n"
		"<member><name>Name</name><value><string>%s</string></value></member>\n"
		"<member><name>Category</name><value><string>%s</string></value></member>\n"
		"<member><name>Priority</name><value><i4>%i</i4></value></member>\n"
		"</struct></value>\n";

	const char* JSON_URLQUEUE_ITEM = 
		"{\n"
		"\"ID\" : %i,\n"
		"\"NZBFilename\" : \"%s\",\n"
		"\"URL\" : \"%s\",\n"
		"\"Name\" : \"%s\",\n"
		"\"Category\" : \"%s\",\n"
		"\"Priority\" : %i\n"
		"}";

	DownloadQueue* pDownloadQueue = DownloadQueue::Lock();

	int iItemBufSize = 10240;
	char* szItemBuf = (char*)malloc(iItemBufSize);
	int index = 0;

	for (NZBList::iterator it = pDownloadQueue->GetQueue()->begin(); it != pDownloadQueue->GetQueue()->end(); it++)
	{
		NZBInfo* pNZBInfo = *it;

		if (pNZBInfo->GetKind() == NZBInfo::nkUrl)
		{
			char* xmlNicename = EncodeStr(pNZBInfo->GetName());
			char* xmlNZBFilename = EncodeStr(pNZBInfo->GetFilename());
			char* xmlURL = EncodeStr(pNZBInfo->GetURL());
			char* xmlCategory = EncodeStr(pNZBInfo->GetCategory());

			snprintf(szItemBuf, iItemBufSize, IsJson() ? JSON_URLQUEUE_ITEM : XML_URLQUEUE_ITEM,
				pNZBInfo->GetID(), xmlNZBFilename, xmlURL, xmlNicename, xmlCategory, pNZBInfo->GetPriority());
			szItemBuf[iItemBufSize-1] = '\0';

			free(xmlNicename);
			free(xmlNZBFilename);
			free(xmlURL);
			free(xmlCategory);

			if (IsJson() && index++ > 0)
			{
				AppendResponse(",\n");
			}
			AppendResponse(szItemBuf);
		}
	}
	free(szItemBuf);

	DownloadQueue::Unlock();

	AppendResponse(IsJson() ? "\n]" : "</data></array>\n");
}

// struct[] config()
void ConfigXmlCommand::Execute()
{
	const char* XML_CONFIG_ITEM = 
		"<value><struct>\n"
		"<member><name>Name</name><value><string>%s</string></value></member>\n"
		"<member><name>Value</name><value><string>%s</string></value></member>\n"
		"</struct></value>\n";

	const char* JSON_CONFIG_ITEM = 
		"{\n"
		"\"Name\" : \"%s\",\n"
		"\"Value\" : \"%s\"\n"
		"}";

	AppendResponse(IsJson() ? "[\n" : "<array><data>\n");

	int iItemBufSize = 1024;
	char* szItemBuf = (char*)malloc(iItemBufSize);
	int index = 0;

	Options::OptEntries* pOptEntries = g_pOptions->LockOptEntries();

	for (Options::OptEntries::iterator it = pOptEntries->begin(); it != pOptEntries->end(); it++)
	{
		Options::OptEntry* pOptEntry = *it;

		char* xmlName = EncodeStr(pOptEntry->GetName());
		char* xmlValue = EncodeStr(pOptEntry->GetValue());

		// option values can sometimes have unlimited length
		int iValLen = strlen(xmlValue);
		if (iValLen > iItemBufSize - 500)
		{
			iItemBufSize = iValLen + 500;
			szItemBuf = (char*)realloc(szItemBuf, iItemBufSize);
		}

		snprintf(szItemBuf, iItemBufSize, IsJson() ? JSON_CONFIG_ITEM : XML_CONFIG_ITEM, xmlName, xmlValue);
		szItemBuf[iItemBufSize-1] = '\0';

		free(xmlName);
		free(xmlValue);

		if (IsJson() && index++ > 0)
		{
			AppendResponse(",\n");
		}
		AppendResponse(szItemBuf);
	}

	g_pOptions->UnlockOptEntries();

	free(szItemBuf);

	AppendResponse(IsJson() ? "\n]" : "</data></array>\n");
}

// struct[] loadconfig()
void LoadConfigXmlCommand::Execute()
{
	const char* XML_CONFIG_ITEM = 
		"<value><struct>\n"
		"<member><name>Name</name><value><string>%s</string></value></member>\n"
		"<member><name>Value</name><value><string>%s</string></value></member>\n"
		"</struct></value>\n";

	const char* JSON_CONFIG_ITEM = 
		"{\n"
		"\"Name\" : \"%s\",\n"
		"\"Value\" : \"%s\"\n"
		"}";

	Options::OptEntries* pOptEntries = new Options::OptEntries();
	if (!g_pOptions->LoadConfig(pOptEntries))
	{
		BuildErrorResponse(3, "Could not read configuration file");
		delete pOptEntries;
		return;
	}

	AppendResponse(IsJson() ? "[\n" : "<array><data>\n");

	int iItemBufSize = 1024;
	char* szItemBuf = (char*)malloc(iItemBufSize);
	int index = 0;

	for (Options::OptEntries::iterator it = pOptEntries->begin(); it != pOptEntries->end(); it++)
	{
		Options::OptEntry* pOptEntry = *it;

		char* xmlName = EncodeStr(pOptEntry->GetName());
		char* xmlValue = EncodeStr(pOptEntry->GetValue());

		// option values can sometimes have unlimited length
		int iValLen = strlen(xmlValue);
		if (iValLen > iItemBufSize - 500)
		{
			iItemBufSize = iValLen + 500;
			szItemBuf = (char*)realloc(szItemBuf, iItemBufSize);
		}

		snprintf(szItemBuf, iItemBufSize, IsJson() ? JSON_CONFIG_ITEM : XML_CONFIG_ITEM, xmlName, xmlValue);
		szItemBuf[iItemBufSize-1] = '\0';

		free(xmlName);
		free(xmlValue);

		if (IsJson() && index++ > 0)
		{
			AppendResponse(",\n");
		}
		AppendResponse(szItemBuf);
	}

	delete pOptEntries;

	free(szItemBuf);

	AppendResponse(IsJson() ? "\n]" : "</data></array>\n");
}

// bool saveconfig(struct[] data)
void SaveConfigXmlCommand::Execute()
{
	Options::OptEntries* pOptEntries = new Options::OptEntries();

	char* szName;
	char* szValue;
	char* szDummy;
	while ((IsJson() && NextParamAsStr(&szDummy) && NextParamAsStr(&szName) &&
			NextParamAsStr(&szDummy) && NextParamAsStr(&szValue)) ||
		   (!IsJson() && NextParamAsStr(&szName) && NextParamAsStr(&szValue)))
	{
		DecodeStr(szName);
		DecodeStr(szValue);
		pOptEntries->push_back(new Options::OptEntry(szName, szValue));
	}

	// save to config file
	bool bOK = g_pOptions->SaveConfig(pOptEntries);

	delete pOptEntries;

	BuildBoolResponse(bOK);
}

// struct[] configtemplates(bool loadFromDisk)
// parameter "loadFromDisk" is optional (new in v14)
void ConfigTemplatesXmlCommand::Execute()
{
	const char* XML_CONFIG_ITEM = 
		"<value><struct>\n"
		"<member><name>Name</name><value><string>%s</string></value></member>\n"
		"<member><name>DisplayName</name><value><string>%s</string></value></member>\n"
		"<member><name>PostScript</name><value><boolean>%s</boolean></value></member>\n"
		"<member><name>ScanScript</name><value><boolean>%s</boolean></value></member>\n"
		"<member><name>QueueScript</name><value><boolean>%s</boolean></value></member>\n"
		"<member><name>SchedulerScript</name><value><boolean>%s</boolean></value></member>\n"
		"<member><name>Template</name><value><string>%s</string></value></member>\n"
		"</struct></value>\n";

	const char* JSON_CONFIG_ITEM = 
		"{\n"
		"\"Name\" : \"%s\",\n"
		"\"DisplayName\" : \"%s\",\n"
		"\"PostScript\" : %s,\n"
		"\"ScanScript\" : %s,\n"
		"\"QueueScript\" : %s,\n"
		"\"SchedulerScript\" : %s,\n"
		"\"Template\" : \"%s\"\n"
		"}";

	bool bLoadFromDisk = false;
	NextParamAsBool(&bLoadFromDisk);

	Options::ConfigTemplates* pConfigTemplates = g_pOptions->GetConfigTemplates();
	
	if (bLoadFromDisk)
	{
		pConfigTemplates = new Options::ConfigTemplates();
		if (!g_pOptions->LoadConfigTemplates(pConfigTemplates))
		{
			BuildErrorResponse(3, "Could not read configuration templates");
			delete pConfigTemplates;
			return;
		}
	}

	AppendResponse(IsJson() ? "[\n" : "<array><data>\n");

	int index = 0;

	for (Options::ConfigTemplates::iterator it = pConfigTemplates->begin(); it != pConfigTemplates->end(); it++)
	{
		Options::ConfigTemplate* pConfigTemplate = *it;

		char* xmlName = EncodeStr(pConfigTemplate->GetScript() ? pConfigTemplate->GetScript()->GetName() : "");
		char* xmlDisplayName = EncodeStr(pConfigTemplate->GetScript() ? pConfigTemplate->GetScript()->GetDisplayName() : "");
		char* xmlTemplate = EncodeStr(pConfigTemplate->GetTemplate());

		int iItemBufSize = strlen(xmlName) + strlen(xmlTemplate) + 1024;
		char* szItemBuf = (char*)malloc(iItemBufSize);

		snprintf(szItemBuf, iItemBufSize, IsJson() ? JSON_CONFIG_ITEM : XML_CONFIG_ITEM,
			xmlName, xmlDisplayName,
			BoolToStr(pConfigTemplate->GetScript() && pConfigTemplate->GetScript()->GetPostScript()),
			BoolToStr(pConfigTemplate->GetScript() && pConfigTemplate->GetScript()->GetScanScript()),
			BoolToStr(pConfigTemplate->GetScript() && pConfigTemplate->GetScript()->GetQueueScript()),
			BoolToStr(pConfigTemplate->GetScript() && pConfigTemplate->GetScript()->GetSchedulerScript()),
			xmlTemplate);
		szItemBuf[iItemBufSize-1] = '\0';

		free(xmlName);
		free(xmlDisplayName);
		free(xmlTemplate);

		if (IsJson() && index++ > 0)
		{
			AppendResponse(",\n");
		}
		AppendResponse(szItemBuf);

		free(szItemBuf);
	}

	if (bLoadFromDisk)
	{
		delete pConfigTemplates;
	}

	AppendResponse(IsJson() ? "\n]" : "</data></array>\n");
}

ViewFeedXmlCommand::ViewFeedXmlCommand(bool bPreview)
{
	m_bPreview = bPreview;
}

// struct[] viewfeed(int id)
// struct[] previewfeed(string name, string url, string filter, bool includeNonMatching)
// struct[] previewfeed(string name, string url, string filter, bool pauseNzb, string category, int priority,
//		bool includeNonMatching, int cacheTimeSec, string cacheId)
void ViewFeedXmlCommand::Execute()
{
	bool bOK = false;
	bool bIncludeNonMatching = false;
	FeedItemInfos* pFeedItemInfos = NULL;

	if (m_bPreview)
	{
		char* szName;
		char* szUrl;
		char* szFilter;
		bool bPauseNzb;
		char* szCategory;
		int iPriority;
		char* szCacheId;
		int iCacheTimeSec;
		if (!NextParamAsStr(&szName) || !NextParamAsStr(&szUrl) || !NextParamAsStr(&szFilter) ||
			!NextParamAsBool(&bPauseNzb) || !NextParamAsStr(&szCategory) || !NextParamAsInt(&iPriority) ||
			!NextParamAsBool(&bIncludeNonMatching) || !NextParamAsInt(&iCacheTimeSec) ||
			!NextParamAsStr(&szCacheId))
		{
			BuildErrorResponse(2, "Invalid parameter");
			return;
		}

		DecodeStr(szName);
		DecodeStr(szUrl);
		DecodeStr(szFilter);
		DecodeStr(szCacheId);
		DecodeStr(szCategory);

		debug("Url=%s", szUrl);
		debug("Filter=%s", szFilter);

		bOK = g_pFeedCoordinator->PreviewFeed(szName, szUrl, szFilter,
			bPauseNzb, szCategory, iPriority, iCacheTimeSec, szCacheId, &pFeedItemInfos);
	}
	else
	{
		int iID = 0;
		if (!NextParamAsInt(&iID) || !NextParamAsBool(&bIncludeNonMatching))
		{
			BuildErrorResponse(2, "Invalid parameter");
			return;
		}

		debug("ID=%i", iID);

		bOK = g_pFeedCoordinator->ViewFeed(iID, &pFeedItemInfos);
	}

	if (!bOK)
	{
		BuildErrorResponse(3, "Could not read feed");
		return;
	}

	const char* XML_FEED_ITEM = 
		"<value><struct>\n"
		"<member><name>Title</name><value><string>%s</string></value></member>\n"
		"<member><name>Filename</name><value><string>%s</string></value></member>\n"
		"<member><name>URL</name><value><string>%s</string></value></member>\n"
		"<member><name>SizeLo</name><value><i4>%i</i4></value></member>\n"
		"<member><name>SizeHi</name><value><i4>%i</i4></value></member>\n"
		"<member><name>SizeMB</name><value><i4>%i</i4></value></member>\n"
		"<member><name>Category</name><value><string>%s</string></value></member>\n"
		"<member><name>AddCategory</name><value><string>%s</string></value></member>\n"
		"<member><name>PauseNzb</name><value><boolean>%s</boolean></value></member>\n"
		"<member><name>Priority</name><value><i4>%i</i4></value></member>\n"
		"<member><name>Time</name><value><i4>%i</i4></value></member>\n"
		"<member><name>Match</name><value><string>%s</string></value></member>\n"
		"<member><name>Rule</name><value><i4>%i</i4></value></member>\n"
		"<member><name>DupeKey</name><value><string>%s</string></value></member>\n"
		"<member><name>DupeScore</name><value><i4>%i</i4></value></member>\n"
		"<member><name>DupeMode</name><value><string>%s</string></value></member>\n"
		"<member><name>Status</name><value><string>%s</string></value></member>\n"
		"</struct></value>\n";

	const char* JSON_FEED_ITEM = 
		"{\n"
		"\"Title\" : \"%s\",\n"
		"\"Filename\" : \"%s\",\n"
		"\"URL\" : \"%s\",\n"
		"\"SizeLo\" : %i,\n"
		"\"SizeHi\" : %i,\n"
		"\"SizeMB\" : %i,\n"
		"\"Category\" : \"%s\",\n"
		"\"AddCategory\" : \"%s\",\n"
		"\"PauseNzb\" : %s,\n"
		"\"Priority\" : %i,\n"
		"\"Time\" : %i,\n"
		"\"Match\" : \"%s\",\n"
		"\"Rule\" : %i,\n"
		"\"DupeKey\" : \"%s\",\n"
		"\"DupeScore\" : %i,\n"
		"\"DupeMode\" : \"%s\",\n"
		"\"Status\" : \"%s\"\n"
		"}";

    const char* szStatusType[] = { "UNKNOWN", "BACKLOG", "FETCHED", "NEW" };
    const char* szMatchStatusType[] = { "IGNORED", "ACCEPTED", "REJECTED" };
    const char* szDupeModeType[] = { "SCORE", "ALL", "FORCE" };

	int iItemBufSize = 10240;
	char* szItemBuf = (char*)malloc(iItemBufSize);

	AppendResponse(IsJson() ? "[\n" : "<array><data>\n");
	int index = 0;

    for (FeedItemInfos::iterator it = pFeedItemInfos->begin(); it != pFeedItemInfos->end(); it++)
    {
        FeedItemInfo* pFeedItemInfo = *it;

		if (bIncludeNonMatching || pFeedItemInfo->GetMatchStatus() == FeedItemInfo::msAccepted)
		{
			unsigned long iSizeHi, iSizeLo;
			Util::SplitInt64(pFeedItemInfo->GetSize(), &iSizeHi, &iSizeLo);
			int iSizeMB = (int)(pFeedItemInfo->GetSize() / 1024 / 1024);

			char* xmltitle = EncodeStr(pFeedItemInfo->GetTitle());
			char* xmlfilename = EncodeStr(pFeedItemInfo->GetFilename());
			char* xmlurl = EncodeStr(pFeedItemInfo->GetUrl());
			char* xmlcategory = EncodeStr(pFeedItemInfo->GetCategory());
			char* xmladdcategory = EncodeStr(pFeedItemInfo->GetAddCategory());
			char* xmldupekey = EncodeStr(pFeedItemInfo->GetDupeKey());

			snprintf(szItemBuf, iItemBufSize, IsJson() ? JSON_FEED_ITEM : XML_FEED_ITEM,
				xmltitle, xmlfilename, xmlurl, iSizeLo, iSizeHi, iSizeMB, xmlcategory, xmladdcategory,
				BoolToStr(pFeedItemInfo->GetPauseNzb()), pFeedItemInfo->GetPriority(), pFeedItemInfo->GetTime(),
				szMatchStatusType[pFeedItemInfo->GetMatchStatus()], pFeedItemInfo->GetMatchRule(),
				xmldupekey, pFeedItemInfo->GetDupeScore(), szDupeModeType[pFeedItemInfo->GetDupeMode()],
				szStatusType[pFeedItemInfo->GetStatus()]);
			szItemBuf[iItemBufSize-1] = '\0';

			free(xmltitle);
			free(xmlfilename);
			free(xmlurl);
			free(xmlcategory);
			free(xmladdcategory);
			free(xmldupekey);

			if (IsJson() && index++ > 0)
			{
				AppendResponse(",\n");
			}
			AppendResponse(szItemBuf);
		}
    }

	free(szItemBuf);

	pFeedItemInfos->Release();

	AppendResponse(IsJson() ? "\n]" : "</data></array>\n");
}

// bool fetchfeed(int ID)
void FetchFeedXmlCommand::Execute()
{
	if (!CheckSafeMethod())
	{
		return;
	}

	int iID;
	if (!NextParamAsInt(&iID))
	{
		BuildErrorResponse(2, "Invalid parameter (ID)");
		return;
	}

	g_pFeedCoordinator->FetchFeed(iID);

	BuildBoolResponse(true);
}

// bool editserver(int ID, bool Active)
void EditServerXmlCommand::Execute()
{
	if (!CheckSafeMethod())
	{
		return;
	}

	bool bOK = false;
	int bFirst = true;

	int iID;
	while (NextParamAsInt(&iID))
	{
		bFirst = false;

		bool bActive;
		if (!NextParamAsBool(&bActive))
		{
			BuildErrorResponse(2, "Invalid parameter");
			return;
		}

		for (Servers::iterator it = g_pServerPool->GetServers()->begin(); it != g_pServerPool->GetServers()->end(); it++)
		{
			NewsServer* pServer = *it;
			if (pServer->GetID() == iID)
			{
				pServer->SetActive(bActive);
				bOK = true;
			}
		}
	}

	if (bFirst)
	{
		BuildErrorResponse(2, "Invalid parameter");
		return;
	}

	if (bOK)
	{
		g_pServerPool->Changed();
	}

	BuildBoolResponse(bOK);
}

// string readurl(string url, string infoname)
void ReadUrlXmlCommand::Execute()
{
	char* szURL;
	if (!NextParamAsStr(&szURL))
	{
		BuildErrorResponse(2, "Invalid parameter (URL)");
		return;
	}
	DecodeStr(szURL);

	char* szInfoName;
	if (!NextParamAsStr(&szInfoName))
	{
		BuildErrorResponse(2, "Invalid parameter (InfoName)");
		return;
	}
	DecodeStr(szInfoName);

	// generate temp file name
	char szTempFileName[1024];
	int iNum = 1;
	while (iNum == 1 || Util::FileExists(szTempFileName))
	{
		snprintf(szTempFileName, 1024, "%sreadurl-%i.tmp", g_pOptions->GetTempDir(), iNum);
		szTempFileName[1024-1] = '\0';
		iNum++;
	}

	WebDownloader* pDownloader = new WebDownloader();
	pDownloader->SetURL(szURL);
	pDownloader->SetForce(true);
	pDownloader->SetRetry(false);
	pDownloader->SetOutputFilename(szTempFileName);
	pDownloader->SetInfoName(szInfoName);

	// do sync download
	WebDownloader::EStatus eStatus = pDownloader->Download();
	bool bOK = eStatus == WebDownloader::adFinished;

	delete pDownloader;

	if (bOK)
	{
		char* szFileContent = NULL;
		int iFileContentLen = 0;
		Util::LoadFileIntoBuffer(szTempFileName, &szFileContent, &iFileContentLen);
		char* xmlContent = EncodeStr(szFileContent);
		free(szFileContent);
		AppendResponse(IsJson() ? "\"" : "<string>");
		AppendResponse(xmlContent);
		AppendResponse(IsJson() ? "\"" : "</string>");
		free(xmlContent);
	}
	else
	{
		BuildErrorResponse(3, "Could not read url");
	}

	remove(szTempFileName);
}

// string checkupdates()
void CheckUpdatesXmlCommand::Execute()
{
	char* szUpdateInfo = NULL;
	bool bOK = g_pMaintenance->CheckUpdates(&szUpdateInfo);

	if (bOK)
	{
		char* xmlContent = EncodeStr(szUpdateInfo);
		free(szUpdateInfo);
		AppendResponse(IsJson() ? "\"" : "<string>");
		AppendResponse(xmlContent);
		AppendResponse(IsJson() ? "\"" : "</string>");
		free(xmlContent);
	}
	else
	{
		BuildErrorResponse(3, "Could not read update info from update-info-script");
	}
}

// bool startupdate(string branch)
void StartUpdateXmlCommand::Execute()
{
	if (!CheckSafeMethod())
	{
		return;
	}

	char* szBranch;
	if (!NextParamAsStr(&szBranch))
	{
		BuildErrorResponse(2, "Invalid parameter (Branch)");
		return;
	}
	DecodeStr(szBranch);

	Maintenance::EBranch eBranch;
	if (!strcasecmp(szBranch, "stable"))
	{
		eBranch = Maintenance::brStable;
	}
	else if (!strcasecmp(szBranch, "testing"))
	{
		eBranch = Maintenance::brTesting;
	}
	else if (!strcasecmp(szBranch, "devel"))
	{
		eBranch = Maintenance::brDevel;
	}
	else
	{
		BuildErrorResponse(2, "Invalid parameter (Branch)");
		return;
	}

	bool bOK = g_pMaintenance->StartUpdate(eBranch);

	BuildBoolResponse(bOK);
}

// struct[] logupdate(idfrom, entries)
Log::Messages* LogUpdateXmlCommand::LockMessages()
{
	return g_pMaintenance->LockMessages();
}

void LogUpdateXmlCommand::UnlockMessages()
{
	g_pMaintenance->UnlockMessages();
}

// struct[] servervolumes()
void ServerVolumesXmlCommand::Execute()
{
	const char* XML_VOLUME_ITEM_START =
	"<value><struct>\n"
	"<member><name>ServerID</name><value><i4>%i</i4></value></member>\n"
	"<member><name>DataTime</name><value><i4>%i</i4></value></member>\n"
	"<member><name>FirstDay</name><value><i4>%i</i4></value></member>\n"
	"<member><name>TotalSizeLo</name><value><i4>%u</i4></value></member>\n"
	"<member><name>TotalSizeHi</name><value><i4>%u</i4></value></member>\n"
	"<member><name>TotalSizeMB</name><value><i4>%i</i4></value></member>\n"
	"<member><name>CustomSizeLo</name><value><i4>%u</i4></value></member>\n"
	"<member><name>CustomSizeHi</name><value><i4>%u</i4></value></member>\n"
	"<member><name>CustomSizeMB</name><value><i4>%i</i4></value></member>\n"
	"<member><name>CustomTime</name><value><i4>%i</i4></value></member>\n"
	"<member><name>SecSlot</name><value><i4>%i</i4></value></member>\n"
	"<member><name>MinSlot</name><value><i4>%i</i4></value></member>\n"
	"<member><name>HourSlot</name><value><i4>%i</i4></value></member>\n"
	"<member><name>DaySlot</name><value><i4>%i</i4></value></member>\n";

	const char* XML_BYTES_ARRAY_START =
	"<member><name>%s</name><value><array><data>\n";

	const char* XML_BYTES_ARRAY_ITEM =
	"<value><struct>\n"
	"<member><name>SizeLo</name><value><i4>%u</i4></value></member>\n"
	"<member><name>SizeHi</name><value><i4>%u</i4></value></member>\n"
	"<member><name>SizeMB</name><value><i4>%i</i4></value></member>\n"
	"</struct></value>\n";

	const char* XML_BYTES_ARRAY_END =
	"</data></array></value></member>\n";

	const char* XML_VOLUME_ITEM_END =
	"</struct></value>\n";

	const char* JSON_VOLUME_ITEM_START =
	"{\n"
	"\"ServerID\" : %i,\n"
	"\"DataTime\" : %i,\n"
	"\"FirstDay\" : %i,\n"
	"\"TotalSizeLo\" : %i,\n"
	"\"TotalSizeHi\" : %i,\n"
	"\"TotalSizeMB\" : %i,\n"
	"\"CustomSizeLo\" : %i,\n"
	"\"CustomSizeHi\" : %i,\n"
	"\"CustomSizeMB\" : %i,\n"
	"\"CustomTime\" : %i,\n"
	"\"SecSlot\" : %i,\n"
	"\"MinSlot\" : %i,\n"
	"\"HourSlot\" : %i,\n"
	"\"DaySlot\" : %i,\n";

	const char* JSON_BYTES_ARRAY_START =
	"\"%s\" : [\n";

	const char* JSON_BYTES_ARRAY_ITEM =
	"{\n"
	"\"SizeLo\" : %u,\n"
	"\"SizeHi\" : %u,\n"
	"\"SizeMB\" : %i\n"
	"}";

	const char* JSON_BYTES_ARRAY_END =
	"]\n";

	const char* JSON_VOLUME_ITEM_END =
	"}\n";

	AppendResponse(IsJson() ? "[\n" : "<array><data>\n");

	ServerVolumes* pServerVolumes = g_pStatMeter->LockServerVolumes();

	const int iItemBufSize = 1024;
	char szItemBuf[iItemBufSize];
	int index = 0;

	for (ServerVolumes::iterator it = pServerVolumes->begin(); it != pServerVolumes->end(); it++, index++)
	{
		ServerVolume* pServerVolume = *it;

		if (IsJson() && index > 0)
		{
			AppendResponse(",\n");
		}

		unsigned long iTotalSizeHi, iTotalSizeLo, iTotalSizeMB;
		Util::SplitInt64(pServerVolume->GetTotalBytes(), &iTotalSizeHi, &iTotalSizeLo);
		iTotalSizeMB = (int)(pServerVolume->GetTotalBytes() / 1024 / 1024);

		unsigned long iCustomSizeHi, iCustomSizeLo, iCustomSizeMB;
		Util::SplitInt64(pServerVolume->GetCustomBytes(), &iCustomSizeHi, &iCustomSizeLo);
		iCustomSizeMB = (int)(pServerVolume->GetCustomBytes() / 1024 / 1024);

		snprintf(szItemBuf, iItemBufSize, IsJson() ? JSON_VOLUME_ITEM_START : XML_VOLUME_ITEM_START,
				 index, (int)pServerVolume->GetDataTime(), pServerVolume->GetFirstDay(),
				 iTotalSizeLo, iTotalSizeHi, iTotalSizeMB, iCustomSizeLo, iCustomSizeHi, iCustomSizeMB, 
				 (int)pServerVolume->GetCustomTime(), pServerVolume->GetSecSlot(),
				 pServerVolume->GetMinSlot(), pServerVolume->GetHourSlot(), pServerVolume->GetDaySlot());
		szItemBuf[iItemBufSize-1] = '\0';
		AppendResponse(szItemBuf);

		ServerVolume::VolumeArray* VolumeArrays[] = { pServerVolume->BytesPerSeconds(),
			pServerVolume->BytesPerMinutes(), pServerVolume->BytesPerHours(), pServerVolume->BytesPerDays() };
		const char* VolumeNames[] = { "BytesPerSeconds", "BytesPerMinutes", "BytesPerHours", "BytesPerDays" };

		for (int i=0; i<4; i++)
		{
			ServerVolume::VolumeArray* pVolumeArray = VolumeArrays[i];
			const char* szArrayName = VolumeNames[i];

			snprintf(szItemBuf, iItemBufSize, IsJson() ? JSON_BYTES_ARRAY_START : XML_BYTES_ARRAY_START, szArrayName);
			szItemBuf[iItemBufSize-1] = '\0';
			AppendResponse(szItemBuf);

			int index2 = 0;
			for (ServerVolume::VolumeArray::iterator it2 = pVolumeArray->begin(); it2 != pVolumeArray->end(); it2++)
			{
				long long lBytes = *it2;
				unsigned long iSizeHi, iSizeLo, iSizeMB;
				Util::SplitInt64(lBytes, &iSizeHi, &iSizeLo);
				iSizeMB = (int)(lBytes / 1024 / 1024);

				snprintf(szItemBuf, iItemBufSize, IsJson() ? JSON_BYTES_ARRAY_ITEM : XML_BYTES_ARRAY_ITEM,
						 iSizeLo, iSizeHi, iSizeMB);
				szItemBuf[iItemBufSize-1] = '\0';

				if (IsJson() && index2++ > 0)
				{
					AppendResponse(",\n");
				}
				AppendResponse(szItemBuf);
			}

			AppendResponse(IsJson() ? JSON_BYTES_ARRAY_END : XML_BYTES_ARRAY_END);
			if (IsJson() && i < 3)
			{
				AppendResponse(",\n");
			}
		}
		AppendResponse(IsJson() ? JSON_VOLUME_ITEM_END : XML_VOLUME_ITEM_END);
	}

	g_pStatMeter->UnlockServerVolumes();

	AppendResponse(IsJson() ? "\n]" : "</data></array>\n");
}

// bool resetservervolume(int serverid, string counter);
void ResetServerVolumeXmlCommand::Execute()
{
	if (!CheckSafeMethod())
	{
		return;
	}

	int iServerId;
	char* szCounter;
	if (!NextParamAsInt(&iServerId) || !NextParamAsStr(&szCounter))
	{
		BuildErrorResponse(2, "Invalid parameter");
		return;
	}

	if (strcmp(szCounter, "CUSTOM"))
	{
		BuildErrorResponse(3, "Invalid Counter");
		return;
	}

	bool bOK = false;
	ServerVolumes* pServerVolumes = g_pStatMeter->LockServerVolumes();
	int index = 0;
	for (ServerVolumes::iterator it = pServerVolumes->begin(); it != pServerVolumes->end(); it++, index++)
	{
		ServerVolume* pServerVolume = *it;
		if (index == iServerId || iServerId == -1)
		{
			pServerVolume->ResetCustom();
			bOK = true;
		}
	}
	g_pStatMeter->UnlockServerVolumes();

	BuildBoolResponse(bOK);
}
