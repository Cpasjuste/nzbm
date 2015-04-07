/*
 *  This file is part of nzbget
 *
 *  Copyright (C) 2004 Sven Henkel <sidddy@users.sourceforge.net>
 *  Copyright (C) 2007-2015 Andrey Prygunkov <hugbug@users.sourceforge.net>
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
 * $Revision: 1245 $
 * $Date: 2015-03-26 23:28:30 +0100 (jeu. 26 mars 2015) $
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
#include <stdarg.h>
#include <sys/stat.h>
#include <set>
#ifdef WIN32
#include <direct.h>
#include <Shlobj.h>
#else
#include <unistd.h>
#include <getopt.h>
#endif

#include "nzbget.h"
#include "Util.h"
#include "Options.h"
#include "Log.h"
#include "ServerPool.h"
#include "NewsServer.h"
#include "MessageBase.h"
#include "DownloadInfo.h"
#include "Scheduler.h"
#include "FeedCoordinator.h"

extern ServerPool* g_pServerPool;
extern Scheduler* g_pScheduler;
extern FeedCoordinator* g_pFeedCoordinator;

#ifdef WIN32
extern void SetupFirstStart();
#endif

#ifdef HAVE_GETOPT_LONG
static struct option long_options[] =
    {
	    {"help", no_argument, 0, 'h'},
	    {"configfile", required_argument, 0, 'c'},
	    {"noconfigfile", no_argument, 0, 'n'},
	    {"printconfig", no_argument, 0, 'p'},
	    {"server", no_argument, 0, 's' },
	    {"daemon", no_argument, 0, 'D' },
	    {"version", no_argument, 0, 'v'},
	    {"serverversion", no_argument, 0, 'V'},
	    {"option", required_argument, 0, 'o'},
	    {"append", no_argument, 0, 'A'},
	    {"list", no_argument, 0, 'L'},
	    {"pause", no_argument, 0, 'P'},
	    {"unpause", no_argument, 0, 'U'},
	    {"rate", required_argument, 0, 'R'},
	    {"system", no_argument, 0, 'B'},
	    {"log", required_argument, 0, 'G'},
	    {"top", no_argument, 0, 'T'},
	    {"edit", required_argument, 0, 'E'},
	    {"connect", no_argument, 0, 'C'},
	    {"quit", no_argument, 0, 'Q'},
	    {"reload", no_argument, 0, 'O'},
	    {"write", required_argument, 0, 'W'},
	    {"category", required_argument, 0, 'K'},
	    {"scan", no_argument, 0, 'S'},
	    {0, 0, 0, 0}
    };
#endif

static char short_options[] = "c:hno:psvAB:DCE:G:K:LPR:STUQOVW:";

// Program options
static const char* OPTION_CONFIGFILE			= "ConfigFile";
static const char* OPTION_APPBIN				= "AppBin";
static const char* OPTION_APPDIR				= "AppDir";
static const char* OPTION_VERSION				= "Version";
static const char* OPTION_MAINDIR				= "MainDir";
static const char* OPTION_DESTDIR				= "DestDir";
static const char* OPTION_INTERDIR				= "InterDir";
static const char* OPTION_TEMPDIR				= "TempDir";
static const char* OPTION_QUEUEDIR				= "QueueDir";
static const char* OPTION_NZBDIR				= "NzbDir";
static const char* OPTION_WEBDIR				= "WebDir";
static const char* OPTION_CONFIGTEMPLATE		= "ConfigTemplate";
static const char* OPTION_SCRIPTDIR				= "ScriptDir";
static const char* OPTION_LOGFILE				= "LogFile";
static const char* OPTION_WRITELOG				= "WriteLog";
static const char* OPTION_ROTATELOG				= "RotateLog";
static const char* OPTION_APPENDCATEGORYDIR		= "AppendCategoryDir";
static const char* OPTION_LOCKFILE				= "LockFile";
static const char* OPTION_DAEMONUSERNAME		= "DaemonUsername";
static const char* OPTION_OUTPUTMODE			= "OutputMode";
static const char* OPTION_DUPECHECK				= "DupeCheck";
static const char* OPTION_DOWNLOADRATE			= "DownloadRate";
static const char* OPTION_CONTROLIP				= "ControlIp";
static const char* OPTION_CONTROLPORT			= "ControlPort";
static const char* OPTION_CONTROLUSERNAME		= "ControlUsername";
static const char* OPTION_CONTROLPASSWORD		= "ControlPassword";
static const char* OPTION_RESTRICTEDUSERNAME	= "RestrictedUsername";
static const char* OPTION_RESTRICTEDPASSWORD	= "RestrictedPassword";
static const char* OPTION_ADDUSERNAME			= "AddUsername";
static const char* OPTION_ADDPASSWORD			= "AddPassword";
static const char* OPTION_SECURECONTROL			= "SecureControl";
static const char* OPTION_SECUREPORT			= "SecurePort";
static const char* OPTION_SECURECERT			= "SecureCert";
static const char* OPTION_SECUREKEY				= "SecureKey";
static const char* OPTION_AUTHORIZEDIP			= "AuthorizedIP";
static const char* OPTION_ARTICLETIMEOUT		= "ArticleTimeout";
static const char* OPTION_URLTIMEOUT			= "UrlTimeout";
static const char* OPTION_SAVEQUEUE				= "SaveQueue";
static const char* OPTION_RELOADQUEUE			= "ReloadQueue";
static const char* OPTION_BROKENLOG				= "BrokenLog";
static const char* OPTION_NZBLOG				= "NzbLog";
static const char* OPTION_DECODE				= "Decode";
static const char* OPTION_RETRIES				= "Retries";
static const char* OPTION_RETRYINTERVAL			= "RetryInterval";
static const char* OPTION_TERMINATETIMEOUT		= "TerminateTimeout";
static const char* OPTION_CONTINUEPARTIAL		= "ContinuePartial";
static const char* OPTION_URLCONNECTIONS		= "UrlConnections";
static const char* OPTION_LOGBUFFERSIZE			= "LogBufferSize";
static const char* OPTION_INFOTARGET			= "InfoTarget";
static const char* OPTION_WARNINGTARGET			= "WarningTarget";
static const char* OPTION_ERRORTARGET			= "ErrorTarget";
static const char* OPTION_DEBUGTARGET			= "DebugTarget";
static const char* OPTION_DETAILTARGET			= "DetailTarget";
static const char* OPTION_PARCHECK				= "ParCheck";
static const char* OPTION_PARREPAIR				= "ParRepair";
static const char* OPTION_PARSCAN				= "ParScan";
static const char* OPTION_PARQUICK				= "ParQuick";
static const char* OPTION_PARRENAME				= "ParRename";
static const char* OPTION_PARBUFFER				= "ParBuffer";
static const char* OPTION_PARTHREADS			= "ParThreads";
static const char* OPTION_HEALTHCHECK			= "HealthCheck";
static const char* OPTION_SCANSCRIPT			= "ScanScript";
static const char* OPTION_QUEUESCRIPT			= "QueueScript";
static const char* OPTION_UMASK					= "UMask";
static const char* OPTION_UPDATEINTERVAL		= "UpdateInterval";
static const char* OPTION_CURSESNZBNAME			= "CursesNzbName";
static const char* OPTION_CURSESTIME			= "CursesTime";
static const char* OPTION_CURSESGROUP			= "CursesGroup";
static const char* OPTION_CRCCHECK				= "CrcCheck";
static const char* OPTION_DIRECTWRITE			= "DirectWrite";
static const char* OPTION_WRITEBUFFER			= "WriteBuffer";
static const char* OPTION_NZBDIRINTERVAL		= "NzbDirInterval";
static const char* OPTION_NZBDIRFILEAGE			= "NzbDirFileAge";
static const char* OPTION_PARCLEANUPQUEUE		= "ParCleanupQueue";
static const char* OPTION_DISKSPACE				= "DiskSpace";
static const char* OPTION_DUMPCORE				= "DumpCore";
static const char* OPTION_PARPAUSEQUEUE			= "ParPauseQueue";
static const char* OPTION_SCRIPTPAUSEQUEUE		= "ScriptPauseQueue";
static const char* OPTION_NZBCLEANUPDISK		= "NzbCleanupDisk";
static const char* OPTION_DELETECLEANUPDISK		= "DeleteCleanupDisk";
static const char* OPTION_PARTIMELIMIT			= "ParTimeLimit";
static const char* OPTION_KEEPHISTORY			= "KeepHistory";
static const char* OPTION_ACCURATERATE			= "AccurateRate";
static const char* OPTION_UNPACK				= "Unpack";
static const char* OPTION_UNPACKCLEANUPDISK		= "UnpackCleanupDisk";
static const char* OPTION_UNRARCMD				= "UnrarCmd";
static const char* OPTION_SEVENZIPCMD			= "SevenZipCmd";
static const char* OPTION_UNPACKPASSFILE		= "UnpackPassFile";
static const char* OPTION_UNPACKPAUSEQUEUE		= "UnpackPauseQueue";
static const char* OPTION_SCRIPTORDER			= "ScriptOrder";
static const char* OPTION_POSTSCRIPT			= "PostScript";
static const char* OPTION_EXTCLEANUPDISK		= "ExtCleanupDisk";
static const char* OPTION_PARIGNOREEXT			= "ParIgnoreExt";
static const char* OPTION_FEEDHISTORY			= "FeedHistory";
static const char* OPTION_URLFORCE				= "UrlForce";
static const char* OPTION_TIMECORRECTION		= "TimeCorrection";
static const char* OPTION_PROPAGATIONDELAY		= "PropagationDelay";
static const char* OPTION_ARTICLECACHE			= "ArticleCache";
static const char* OPTION_EVENTINTERVAL			= "EventInterval";

// obsolete options
static const char* OPTION_POSTLOGKIND			= "PostLogKind";
static const char* OPTION_NZBLOGKIND			= "NZBLogKind";
static const char* OPTION_RETRYONCRCERROR		= "RetryOnCrcError";
static const char* OPTION_ALLOWREPROCESS		= "AllowReProcess";
static const char* OPTION_POSTPROCESS			= "PostProcess";
static const char* OPTION_LOADPARS				= "LoadPars";
static const char* OPTION_THREADLIMIT			= "ThreadLimit";
static const char* OPTION_PROCESSLOGKIND		= "ProcessLogKind";
static const char* OPTION_APPENDNZBDIR			= "AppendNzbDir";
static const char* OPTION_RENAMEBROKEN			= "RenameBroken";
static const char* OPTION_MERGENZB				= "MergeNzb";
static const char* OPTION_STRICTPARNAME			= "StrictParName";
static const char* OPTION_RELOADURLQUEUE		= "ReloadUrlQueue";
static const char* OPTION_RELOADPOSTQUEUE		= "ReloadPostQueue";
static const char* OPTION_NZBPROCESS			= "NZBProcess";
static const char* OPTION_NZBADDEDPROCESS		= "NZBAddedProcess";
static const char* OPTION_CREATELOG				= "CreateLog";
static const char* OPTION_RESETLOG				= "ResetLog";

const char* BoolNames[] = { "yes", "no", "true", "false", "1", "0", "on", "off", "enable", "disable", "enabled", "disabled" };
const int BoolValues[] = { 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0 };
const int BoolCount = 12;

static const char* BEGIN_SCRIPT_SIGNATURE = "### NZBGET ";
static const char* POST_SCRIPT_SIGNATURE = "POST-PROCESSING";
static const char* SCAN_SCRIPT_SIGNATURE = "SCAN";
static const char* QUEUE_SCRIPT_SIGNATURE = "QUEUE";
static const char* SCHEDULER_SCRIPT_SIGNATURE = "SCHEDULER";
static const char* END_SCRIPT_SIGNATURE = " SCRIPT";
static const char* QUEUE_EVENTS_SIGNATURE = "### QUEUE EVENTS:";

#ifndef WIN32
const char* PossibleConfigLocations[] =
	{
		"~/.nzbget",
		"/etc/nzbget.conf",
		"/usr/etc/nzbget.conf",
		"/usr/local/etc/nzbget.conf",
		"/opt/etc/nzbget.conf",
		NULL
	};
#endif

Options::OptEntry::OptEntry()
{
	m_szName = NULL;
	m_szValue = NULL;
	m_szDefValue = NULL;
	m_iLineNo = 0;
}

Options::OptEntry::OptEntry(const char* szName, const char* szValue)
{
	m_szName = strdup(szName);
	m_szValue = strdup(szValue);
	m_szDefValue = NULL;
	m_iLineNo = 0;
}

Options::OptEntry::~OptEntry()
{
	free(m_szName);
	free(m_szValue);
	free(m_szDefValue);
}

void Options::OptEntry::SetName(const char* szName)
{
	free(m_szName);
	m_szName = strdup(szName);
}

void Options::OptEntry::SetValue(const char* szValue)
{
	free(m_szValue);
	m_szValue = strdup(szValue);

	if (!m_szDefValue)
	{
		m_szDefValue = strdup(szValue);
	}
}

bool Options::OptEntry::Restricted()
{
	char szLoName[256];
	strncpy(szLoName, m_szName, sizeof(szLoName));
	szLoName[256-1] = '\0';
	for (char* p = szLoName; *p; p++) *p = tolower(*p); // convert string to lowercase

	bool bRestricted = !strcasecmp(m_szName, OPTION_CONTROLIP) ||
		!strcasecmp(m_szName, OPTION_CONTROLPORT) ||
		!strcasecmp(m_szName, OPTION_SECURECONTROL) ||
		!strcasecmp(m_szName, OPTION_SECUREPORT) ||
		!strcasecmp(m_szName, OPTION_SECURECERT) ||
		!strcasecmp(m_szName, OPTION_SECUREKEY) ||
		!strcasecmp(m_szName, OPTION_AUTHORIZEDIP) ||
		!strcasecmp(m_szName, OPTION_DAEMONUSERNAME) ||
		!strcasecmp(m_szName, OPTION_UMASK) ||
		strchr(m_szName, ':') ||			// All extension script options
		strstr(szLoName, "username") ||		// ServerX.Username, ControlUsername, etc.
		strstr(szLoName, "password");		// ServerX.Password, ControlPassword, etc.

	return bRestricted;
}

Options::OptEntries::~OptEntries()
{
	for (iterator it = begin(); it != end(); it++)
	{
		delete *it;
	}
}

Options::OptEntry* Options::OptEntries::FindOption(const char* szName)
{
	if (!szName)
	{
		return NULL;
	}

	for (iterator it = begin(); it != end(); it++)
	{
		OptEntry* pOptEntry = *it;
		if (!strcasecmp(pOptEntry->GetName(), szName))
		{
			return pOptEntry;
		}
	}

	return NULL;
}


Options::ConfigTemplate::ConfigTemplate(Script* pScript, const char* szTemplate)
{
	m_pScript = pScript;
	m_szTemplate = strdup(szTemplate ? szTemplate : "");
}

Options::ConfigTemplate::~ConfigTemplate()
{
	delete m_pScript;
	free(m_szTemplate);
}

Options::ConfigTemplates::~ConfigTemplates()
{
	for (iterator it = begin(); it != end(); it++)
	{
		delete *it;
	}
}


Options::Category::Category(const char* szName, const char* szDestDir, bool bUnpack, const char* szPostScript)
{
	m_szName = strdup(szName);
	m_szDestDir = szDestDir ? strdup(szDestDir) : NULL;
	m_bUnpack = bUnpack;
	m_szPostScript = szPostScript ? strdup(szPostScript) : NULL;
}

Options::Category::~Category()
{
	free(m_szName);
	free(m_szDestDir);
	free(m_szPostScript);

	for (NameList::iterator it = m_Aliases.begin(); it != m_Aliases.end(); it++)
	{
		free(*it);
	}
}

Options::Categories::~Categories()
{
	for (iterator it = begin(); it != end(); it++)
	{
		delete *it;
	}
}

Options::Category* Options::Categories::FindCategory(const char* szName, bool bSearchAliases)
{
	if (!szName)
	{
		return NULL;
	}

	for (iterator it = begin(); it != end(); it++)
	{
		Category* pCategory = *it;
		if (!strcasecmp(pCategory->GetName(), szName))
		{
			return pCategory;
		}
	}

	if (bSearchAliases)
	{
		for (iterator it = begin(); it != end(); it++)
		{
			Category* pCategory = *it;
			for (NameList::iterator it2 = pCategory->GetAliases()->begin(); it2 != pCategory->GetAliases()->end(); it2++)
			{
				const char* szAlias = *it2;
				WildMask mask(szAlias);
				if (mask.Match(szName))
				{
					return pCategory;
				}
			}
		}
	}

	return NULL;
}


Options::Script::Script(const char* szName, const char* szLocation)
{
	m_szName = strdup(szName);
	m_szLocation = strdup(szLocation);
	m_szDisplayName = strdup(szName);
	m_bPostScript = false;
	m_bScanScript = false;
	m_bQueueScript = false;
	m_bSchedulerScript = false;
	m_szQueueEvents = NULL;
}

Options::Script::~Script()
{
	free(m_szName);
	free(m_szLocation);
	free(m_szDisplayName);
	free(m_szQueueEvents);
}

void Options::Script::SetDisplayName(const char* szDisplayName)
{
	free(m_szDisplayName);
	m_szDisplayName = strdup(szDisplayName);
}

void Options::Script::SetQueueEvents(const char* szQueueEvents)
{
	free(m_szQueueEvents);
	m_szQueueEvents = szQueueEvents ? strdup(szQueueEvents) : NULL;
}


Options::Scripts::~Scripts()
{
	Clear();
}

void Options::Scripts::Clear()
{
	for (iterator it = begin(); it != end(); it++)
	{
		delete *it;
	}
	clear();
}

Options::Script* Options::Scripts::Find(const char* szName)
{
	for (iterator it = begin(); it != end(); it++)
	{
		Script* pScript = *it;
		if (!strcmp(pScript->GetName(), szName))
		{
			return pScript;
		}
	}

	return NULL;
}


Options::Options()
{
	m_bConfigErrors = false;
	m_iConfigLine = 0;

	// initialize options with default values
	m_bConfigInitialized	= false;
	m_szConfigFilename		= NULL;
	m_szAppDir				= NULL;
	m_szDestDir				= NULL;
	m_szInterDir			= NULL;
	m_szTempDir				= NULL;
	m_szQueueDir			= NULL;
	m_szNzbDir				= NULL;
	m_szWebDir				= NULL;
	m_szConfigTemplate		= NULL;
	m_szScriptDir			= NULL;
	m_eInfoTarget			= mtScreen;
	m_eWarningTarget		= mtScreen;
	m_eErrorTarget			= mtScreen;
	m_eDebugTarget			= mtScreen;
	m_eDetailTarget			= mtScreen;
	m_bDecode				= true;
	m_bPauseDownload		= false;
	m_bPausePostProcess		= false;
	m_bPauseScan			= false;
	m_bTempPauseDownload	= false;
	m_bBrokenLog			= false;
	m_bNzbLog				= false;
	m_iDownloadRate			= 0;
	m_iEditQueueAction		= 0;
	m_pEditQueueIDList		= NULL;
	m_iEditQueueIDCount		= 0;
	m_iEditQueueOffset		= 0;
	m_szEditQueueText		= NULL;
	m_szArgFilename			= NULL;
	m_szLastArg				= NULL;
	m_szAddCategory			= NULL;
	m_iAddPriority			= 0;
	m_szAddNZBFilename		= NULL;
	m_bAddPaused			= false;
	m_iArticleTimeout		= 0;
	m_iUrlTimeout			= 0;
	m_iTerminateTimeout		= 0;
	m_bServerMode			= false;
	m_bDaemonMode			= false;
	m_bRemoteClientMode		= false;
	m_bPrintOptions			= false;
	m_bAddTop				= false;
	m_szAddDupeKey			= NULL;
	m_iAddDupeScore			= 0;
	m_iAddDupeMode			= 0;
	m_bAppendCategoryDir	= false;
	m_bContinuePartial		= false;
	m_bSaveQueue			= false;
	m_bDupeCheck			= false;
	m_iRetries				= 0;
	m_iRetryInterval		= 0;
	m_iControlPort			= 0;
	m_szControlIP			= NULL;
	m_szControlUsername		= NULL;
	m_szControlPassword		= NULL;
	m_szRestrictedUsername	= NULL;
	m_szRestrictedPassword	= NULL;
	m_szAddUsername			= NULL;
	m_szAddPassword			= NULL;
	m_bSecureControl		= false;
	m_iSecurePort			= 0;
	m_szSecureCert			= NULL;
	m_szSecureKey			= NULL;
	m_szAuthorizedIP		= NULL;
	m_szLockFile			= NULL;
	m_szDaemonUsername		= NULL;
	m_eOutputMode			= omLoggable;
	m_bReloadQueue			= false;
	m_iUrlConnections		= 0;
	m_iLogBufferSize		= 0;
	m_iLogLines				= 0;
	m_iWriteLogKind			= 0;
	m_eWriteLog				= wlAppend;
	m_iRotateLog			= 0;
	m_szLogFile				= NULL;
	m_eParCheck				= pcManual;
	m_bParRepair			= false;
	m_eParScan				= psLimited;
	m_bParQuick				= true;
	m_bParRename			= false;
	m_iParBuffer			= 0;
	m_iParThreads			= 0;
	m_eHealthCheck			= hcNone;
	m_szScriptOrder			= NULL;
	m_szPostScript			= NULL;
	m_szScanScript			= NULL;
	m_szQueueScript			= NULL;
	m_bNoConfig				= false;
	m_iUMask				= 0;
	m_iUpdateInterval		= 0;
	m_bCursesNZBName		= false;
	m_bCursesTime			= false;
	m_bCursesGroup			= false;
	m_bCrcCheck				= false;
	m_bDirectWrite			= false;
	m_iWriteBuffer			= 0;
	m_iNzbDirInterval		= 0;
	m_iNzbDirFileAge		= 0;
	m_bParCleanupQueue		= false;
	m_iDiskSpace			= 0;
	m_bTestBacktrace		= false;
	m_bWebGet				= false;
	m_szWebGetFilename		= NULL;
	m_bTLS					= false;
	m_bDumpCore				= false;
	m_bParPauseQueue		= false;
	m_bScriptPauseQueue		= false;
	m_bNzbCleanupDisk		= false;
	m_bDeleteCleanupDisk	= false;
	m_iParTimeLimit			= 0;
	m_iKeepHistory			= 0;
	m_bAccurateRate			= false;
	m_EMatchMode			= mmID;
	m_tResumeTime			= 0;
	m_bUnpack				= false;
	m_bUnpackCleanupDisk	= false;
	m_szUnrarCmd			= NULL;
	m_szSevenZipCmd			= NULL;
	m_szUnpackPassFile		= NULL;
	m_bUnpackPauseQueue		= false;
	m_szExtCleanupDisk		= NULL;
	m_szParIgnoreExt		= NULL;
	m_iFeedHistory			= 0;
	m_bUrlForce				= false;
	m_iTimeCorrection		= 0;
	m_iLocalTimeOffset		= 0;
	m_iPropagationDelay		= 0;
	m_iArticleCache			= 0;
	m_iEventInterval		= 0;
}

Options::~Options()
{
	free(m_szConfigFilename);
	free(m_szAppDir);
	free(m_szDestDir);
	free(m_szInterDir);
	free(m_szTempDir);
	free(m_szQueueDir);
	free(m_szNzbDir);
	free(m_szWebDir);
	free(m_szConfigTemplate);
	free(m_szScriptDir);
	free(m_szArgFilename);
	free(m_szAddCategory);
	free(m_szEditQueueText);
	free(m_szLastArg);
	free(m_szControlIP);
	free(m_szControlUsername);
	free(m_szControlPassword);
	free(m_szRestrictedUsername);
	free(m_szRestrictedPassword);
	free(m_szAddUsername);
	free(m_szAddPassword);
	free(m_szSecureCert);
	free(m_szSecureKey);
	free(m_szAuthorizedIP);
	free(m_szLogFile);
	free(m_szLockFile);
	free(m_szDaemonUsername);
	free(m_szScriptOrder);
	free(m_szPostScript);
	free(m_szScanScript);
	free(m_szQueueScript);
	free(m_pEditQueueIDList);
	free(m_szAddNZBFilename);
	free(m_szAddDupeKey);
	free(m_szUnrarCmd);
	free(m_szSevenZipCmd);
	free(m_szUnpackPassFile);
	free(m_szExtCleanupDisk);
	free(m_szParIgnoreExt);
	free(m_szWebGetFilename);

	for (NameList::iterator it = m_EditQueueNameList.begin(); it != m_EditQueueNameList.end(); it++)
	{
		free(*it);
	}
	m_EditQueueNameList.clear();
}

void Options::Init(int argc, char* argv[])
{
	// Option "ConfigFile" will be initialized later, but we want
	// to see it at the top of option list, so we add it first
	SetOption(OPTION_CONFIGFILE, "");

	char szFilename[MAX_PATH + 1];
#ifdef WIN32
	GetModuleFileName(NULL, szFilename, sizeof(szFilename));
#else
	Util::ExpandFileName(argv[0], szFilename, sizeof(szFilename));
#endif
	Util::NormalizePathSeparators(szFilename);
	SetOption(OPTION_APPBIN, szFilename);
	char* end = strrchr(szFilename, PATH_SEPARATOR);
	if (end) *end = '\0';
	SetOption(OPTION_APPDIR, szFilename);
	m_szAppDir = strdup(szFilename);

	SetOption(OPTION_VERSION, Util::VersionRevision());

	InitDefault();
	InitCommandLine(argc, argv);

	if (argc == 1)
	{
		PrintUsage(argv[0]);
	}
	if (!m_szConfigFilename && !m_bNoConfig)
	{
		if (argc == 1)
		{
			printf("\n");
		}
		printf("No configuration-file found\n");
#ifdef WIN32
		printf("Please put configuration-file \"nzbget.conf\" into the directory with exe-file\n");
#else
		printf("Please use option \"-c\" or put configuration-file in one of the following locations:\n");
		int p = 0;
		while (const char* szFilename = PossibleConfigLocations[p++])
		{
			printf("%s\n", szFilename);
		}
#endif
		exit(-1);
	}
	if (argc == 1)
	{
		exit(-1);
	}

	InitOptions();
	CheckOptions();

	if (!m_bPrintOptions)
	{
		InitFileArg(argc, argv);
	}

	InitServers();
	InitCategories();
	InitScheduler();
	InitFeeds();
	InitScripts();
	InitConfigTemplates();

	if (m_bPrintOptions)
	{
		Dump();
		exit(-1);
	}

	if (m_bConfigErrors && m_eClientOperation == opClientNoOperation)
	{
		info("Pausing all activities due to errors in configuration");
		m_bPauseDownload = true;
		m_bPausePostProcess = true;
		m_bPauseScan = true;
	}
}

void Options::Dump()
{
	for (OptEntries::iterator it = m_OptEntries.begin(); it != m_OptEntries.end(); it++)
	{
		OptEntry* pOptEntry = *it;
		printf("%s = \"%s\"\n", pOptEntry->GetName(), pOptEntry->GetValue());
	}
}

void Options::ConfigError(const char* msg, ...)
{
	char tmp2[1024];

	va_list ap;
	va_start(ap, msg);
	vsnprintf(tmp2, 1024, msg, ap);
	tmp2[1024-1] = '\0';
	va_end(ap);

	printf("%s(%i): %s\n", Util::BaseFileName(m_szConfigFilename), m_iConfigLine, tmp2);
	error("%s(%i): %s", Util::BaseFileName(m_szConfigFilename), m_iConfigLine, tmp2);

	m_bConfigErrors = true;
}

void Options::ConfigWarn(const char* msg, ...)
{
	char tmp2[1024];
	
	va_list ap;
	va_start(ap, msg);
	vsnprintf(tmp2, 1024, msg, ap);
	tmp2[1024-1] = '\0';
	va_end(ap);
	
	printf("%s(%i): %s\n", Util::BaseFileName(m_szConfigFilename), m_iConfigLine, tmp2);
	warn("%s(%i): %s", Util::BaseFileName(m_szConfigFilename), m_iConfigLine, tmp2);
}

void Options::LocateOptionSrcPos(const char *szOptionName)
{
	OptEntry* pOptEntry = FindOption(szOptionName);
	if (pOptEntry)
	{
		m_iConfigLine = pOptEntry->GetLineNo();
	}
	else
	{
		m_iConfigLine = 0;
	}
}

void Options::InitDefault()
{
#ifdef WIN32
	SetOption(OPTION_MAINDIR, "${AppDir}");
	SetOption(OPTION_WEBDIR, "${AppDir}\\webui");
	SetOption(OPTION_CONFIGTEMPLATE, "${AppDir}\\nzbget.conf.template");
#else
	SetOption(OPTION_MAINDIR, "~/downloads");
	SetOption(OPTION_WEBDIR, "");
	SetOption(OPTION_CONFIGTEMPLATE, "");
#endif
	SetOption(OPTION_TEMPDIR, "${MainDir}/tmp");
	SetOption(OPTION_DESTDIR, "${MainDir}/dst");
	SetOption(OPTION_INTERDIR, "");
	SetOption(OPTION_QUEUEDIR, "${MainDir}/queue");
	SetOption(OPTION_NZBDIR, "${MainDir}/nzb");
	SetOption(OPTION_LOCKFILE, "${MainDir}/nzbget.lock");
	SetOption(OPTION_LOGFILE, "${DestDir}/nzbget.log");
	SetOption(OPTION_SCRIPTDIR, "${MainDir}/scripts");
	SetOption(OPTION_WRITELOG, "append");
	SetOption(OPTION_ROTATELOG, "3");
	SetOption(OPTION_APPENDCATEGORYDIR, "yes");
	SetOption(OPTION_OUTPUTMODE, "curses");
	SetOption(OPTION_DUPECHECK, "yes");
	SetOption(OPTION_DOWNLOADRATE, "0");
	SetOption(OPTION_CONTROLIP, "0.0.0.0");
	SetOption(OPTION_CONTROLUSERNAME, "nzbget");
	SetOption(OPTION_CONTROLPASSWORD, "tegbzn6789");
	SetOption(OPTION_RESTRICTEDUSERNAME, "");
	SetOption(OPTION_RESTRICTEDPASSWORD, "");
	SetOption(OPTION_ADDUSERNAME, "");
	SetOption(OPTION_ADDPASSWORD, "");
	SetOption(OPTION_CONTROLPORT, "6789");
	SetOption(OPTION_SECURECONTROL, "no");
	SetOption(OPTION_SECUREPORT, "6791");
	SetOption(OPTION_SECURECERT, "");
	SetOption(OPTION_SECUREKEY, "");
	SetOption(OPTION_AUTHORIZEDIP, "");
	SetOption(OPTION_ARTICLETIMEOUT, "60");
	SetOption(OPTION_URLTIMEOUT, "60");
	SetOption(OPTION_SAVEQUEUE, "yes");
	SetOption(OPTION_RELOADQUEUE, "yes");
	SetOption(OPTION_BROKENLOG, "yes");
	SetOption(OPTION_NZBLOG, "yes");
	SetOption(OPTION_DECODE, "yes");
	SetOption(OPTION_RETRIES, "3");
	SetOption(OPTION_RETRYINTERVAL, "10");
	SetOption(OPTION_TERMINATETIMEOUT, "600");
	SetOption(OPTION_CONTINUEPARTIAL, "no");
	SetOption(OPTION_URLCONNECTIONS, "4");
	SetOption(OPTION_LOGBUFFERSIZE, "1000");
	SetOption(OPTION_INFOTARGET, "both");
	SetOption(OPTION_WARNINGTARGET, "both");
	SetOption(OPTION_ERRORTARGET, "both");
	SetOption(OPTION_DEBUGTARGET, "none");
	SetOption(OPTION_DETAILTARGET, "both");
	SetOption(OPTION_PARCHECK, "auto");
	SetOption(OPTION_PARREPAIR, "yes");
	SetOption(OPTION_PARSCAN, "limited");
	SetOption(OPTION_PARQUICK, "yes");
	SetOption(OPTION_PARRENAME, "yes");
	SetOption(OPTION_PARBUFFER, "16");
	SetOption(OPTION_PARTHREADS, "1");
	SetOption(OPTION_HEALTHCHECK, "none");
	SetOption(OPTION_SCRIPTORDER, "");
	SetOption(OPTION_POSTSCRIPT, "");
	SetOption(OPTION_SCANSCRIPT, "");
	SetOption(OPTION_QUEUESCRIPT, "");
	SetOption(OPTION_DAEMONUSERNAME, "root");
	SetOption(OPTION_UMASK, "1000");
	SetOption(OPTION_UPDATEINTERVAL, "200");
	SetOption(OPTION_CURSESNZBNAME, "yes");
	SetOption(OPTION_CURSESTIME, "no");
	SetOption(OPTION_CURSESGROUP, "no");
	SetOption(OPTION_CRCCHECK, "yes");
	SetOption(OPTION_DIRECTWRITE, "yes");
	SetOption(OPTION_WRITEBUFFER, "0");
	SetOption(OPTION_NZBDIRINTERVAL, "5");
	SetOption(OPTION_NZBDIRFILEAGE, "60");
	SetOption(OPTION_PARCLEANUPQUEUE, "yes");
	SetOption(OPTION_DISKSPACE, "250");
	SetOption(OPTION_DUMPCORE, "no");
	SetOption(OPTION_PARPAUSEQUEUE, "no");
	SetOption(OPTION_SCRIPTPAUSEQUEUE, "no");
	SetOption(OPTION_NZBCLEANUPDISK, "no");
	SetOption(OPTION_DELETECLEANUPDISK, "no");
	SetOption(OPTION_PARTIMELIMIT, "0");
	SetOption(OPTION_KEEPHISTORY, "7");
	SetOption(OPTION_ACCURATERATE, "no");
	SetOption(OPTION_UNPACK, "no");
	SetOption(OPTION_UNPACKCLEANUPDISK, "no");
#ifdef WIN32
	SetOption(OPTION_UNRARCMD, "unrar.exe");
	SetOption(OPTION_SEVENZIPCMD, "7z.exe");
#else
	SetOption(OPTION_UNRARCMD, "unrar");
	SetOption(OPTION_SEVENZIPCMD, "7z");
#endif
	SetOption(OPTION_UNPACKPASSFILE, "");
	SetOption(OPTION_UNPACKPAUSEQUEUE, "no");
	SetOption(OPTION_EXTCLEANUPDISK, "");
	SetOption(OPTION_PARIGNOREEXT, "");
	SetOption(OPTION_FEEDHISTORY, "7");
	SetOption(OPTION_URLFORCE, "yes");
	SetOption(OPTION_TIMECORRECTION, "0");
	SetOption(OPTION_PROPAGATIONDELAY, "0");
	SetOption(OPTION_ARTICLECACHE, "0");
	SetOption(OPTION_EVENTINTERVAL, "0");
}

void Options::InitOptFile()
{
	if (m_bConfigInitialized)
	{
		return;
	}

	if (!m_szConfigFilename && !m_bNoConfig)
	{
		// search for config file in default locations
#ifdef WIN32
		char szFilename[MAX_PATH + 20];
		snprintf(szFilename, sizeof(szFilename), "%s\\nzbget.conf", m_szAppDir);

		if (!Util::FileExists(szFilename))
		{
			char szAppDataPath[MAX_PATH];
			SHGetFolderPath(NULL, CSIDL_COMMON_APPDATA, NULL, 0, szAppDataPath);
			snprintf(szFilename, sizeof(szFilename), "%s\\NZBGet\\nzbget.conf", szAppDataPath);
			szFilename[sizeof(szFilename)-1] = '\0';

			if (!Util::FileExists(szFilename))
			{
				SetupFirstStart();
			}
		}

		if (Util::FileExists(szFilename))
		{
			m_szConfigFilename = strdup(szFilename);
		}
#else
		int p = 0;
		while (const char* szFilename = PossibleConfigLocations[p++])
		{
			// substitute HOME-variable
			char szExpandedFilename[1024];
			if (Util::ExpandHomePath(szFilename, szExpandedFilename, sizeof(szExpandedFilename)))
			{
				szFilename = szExpandedFilename;
			}

			if (Util::FileExists(szFilename))
			{
				m_szConfigFilename = strdup(szFilename);
				break;
			}
		}
#endif
	}

	if (m_szConfigFilename)
	{
		// normalize path in filename
		char szFilename[MAX_PATH + 1];
		Util::ExpandFileName(m_szConfigFilename, szFilename, sizeof(szFilename));
		szFilename[MAX_PATH] = '\0';

#ifndef WIN32
		// substitute HOME-variable
		char szExpandedFilename[1024];
		if (Util::ExpandHomePath(szFilename, szExpandedFilename, sizeof(szExpandedFilename)))
		{
			strncpy(szFilename, szExpandedFilename, sizeof(szFilename));
		}
#endif
		
		free(m_szConfigFilename);
		m_szConfigFilename = strdup(szFilename);

		SetOption(OPTION_CONFIGFILE, m_szConfigFilename);
		LoadConfigFile();
	}

	m_bConfigInitialized = true;
}

void Options::CheckDir(char** dir, const char* szOptionName,
	const char* szParentDir, bool bAllowEmpty, bool bCreate)
{
	char* usedir = NULL;
	const char* tempdir = GetOption(szOptionName);

	if (Util::EmptyStr(tempdir))
	{
		if (!bAllowEmpty)
		{
			ConfigError("Invalid value for option \"%s\": <empty>", szOptionName);
		}
		*dir = strdup("");
		return;
	}

	int len = strlen(tempdir);
	usedir = (char*)malloc(len + 2);
	strcpy(usedir, tempdir);
	char ch = usedir[len-1];
	Util::NormalizePathSeparators(usedir);
	if (ch != PATH_SEPARATOR)
	{
		usedir[len] = PATH_SEPARATOR;
		usedir[len + 1] = '\0';
	}

	if (!(usedir[0] == PATH_SEPARATOR || usedir[0] == ALT_PATH_SEPARATOR ||
		(usedir[0] && usedir[1] == ':')) &&
		!Util::EmptyStr(szParentDir))
	{
		// convert relative path to absolute path
		int plen = strlen(szParentDir);
		int len2 = len + plen + 4;
		char* usedir2 = (char*)malloc(len2);
		if (szParentDir[plen-1] == PATH_SEPARATOR || szParentDir[plen-1] == ALT_PATH_SEPARATOR)
		{
			snprintf(usedir2, len2, "%s%s", szParentDir, usedir);
		}
		else
		{
			snprintf(usedir2, len2, "%s%c%s", szParentDir, PATH_SEPARATOR, usedir);
		}
		usedir2[len2-1] = '\0';
		free(usedir);

		usedir = usedir2;
		Util::NormalizePathSeparators(usedir);

		int ulen = strlen(usedir);
		usedir[ulen-1] = '\0';
		SetOption(szOptionName, usedir);
		usedir[ulen-1] = PATH_SEPARATOR;
	}

	// Ensure the dir is created
	char szErrBuf[1024];
	if (bCreate && !Util::ForceDirectories(usedir, szErrBuf, sizeof(szErrBuf)))
	{
		ConfigError("Invalid value for option \"%s\" (%s): %s", szOptionName, usedir, szErrBuf);
	}
	*dir = usedir;
}

void Options::InitOptions()
{
	const char* szMainDir = GetOption(OPTION_MAINDIR);

	CheckDir(&m_szDestDir, OPTION_DESTDIR, szMainDir, false, false);
	CheckDir(&m_szInterDir, OPTION_INTERDIR, szMainDir, true, true);
	CheckDir(&m_szTempDir, OPTION_TEMPDIR, szMainDir, false, true);
	CheckDir(&m_szQueueDir, OPTION_QUEUEDIR, szMainDir, false, true);
	CheckDir(&m_szWebDir, OPTION_WEBDIR, NULL, true, false);
	CheckDir(&m_szScriptDir, OPTION_SCRIPTDIR, szMainDir, true, false);

	m_szConfigTemplate		= strdup(GetOption(OPTION_CONFIGTEMPLATE));
	m_szScriptOrder			= strdup(GetOption(OPTION_SCRIPTORDER));
	m_szPostScript			= strdup(GetOption(OPTION_POSTSCRIPT));
	m_szScanScript			= strdup(GetOption(OPTION_SCANSCRIPT));
	m_szQueueScript			= strdup(GetOption(OPTION_QUEUESCRIPT));
	m_szControlIP			= strdup(GetOption(OPTION_CONTROLIP));
	m_szControlUsername		= strdup(GetOption(OPTION_CONTROLUSERNAME));
	m_szControlPassword		= strdup(GetOption(OPTION_CONTROLPASSWORD));
	m_szRestrictedUsername	= strdup(GetOption(OPTION_RESTRICTEDUSERNAME));
	m_szRestrictedPassword	= strdup(GetOption(OPTION_RESTRICTEDPASSWORD));
	m_szAddUsername			= strdup(GetOption(OPTION_ADDUSERNAME));
	m_szAddPassword			= strdup(GetOption(OPTION_ADDPASSWORD));
	m_szSecureCert			= strdup(GetOption(OPTION_SECURECERT));
	m_szSecureKey			= strdup(GetOption(OPTION_SECUREKEY));
	m_szAuthorizedIP		= strdup(GetOption(OPTION_AUTHORIZEDIP));
	m_szLockFile			= strdup(GetOption(OPTION_LOCKFILE));
	m_szDaemonUsername		= strdup(GetOption(OPTION_DAEMONUSERNAME));
	m_szLogFile				= strdup(GetOption(OPTION_LOGFILE));
	m_szUnrarCmd			= strdup(GetOption(OPTION_UNRARCMD));
	m_szSevenZipCmd			= strdup(GetOption(OPTION_SEVENZIPCMD));
	m_szUnpackPassFile		= strdup(GetOption(OPTION_UNPACKPASSFILE));
	m_szExtCleanupDisk		= strdup(GetOption(OPTION_EXTCLEANUPDISK));
	m_szParIgnoreExt		= strdup(GetOption(OPTION_PARIGNOREEXT));

	m_iDownloadRate			= (int)(ParseFloatValue(OPTION_DOWNLOADRATE) * 1024);
	m_iArticleTimeout		= ParseIntValue(OPTION_ARTICLETIMEOUT, 10);
	m_iUrlTimeout			= ParseIntValue(OPTION_URLTIMEOUT, 10);
	m_iTerminateTimeout		= ParseIntValue(OPTION_TERMINATETIMEOUT, 10);
	m_iRetries				= ParseIntValue(OPTION_RETRIES, 10);
	m_iRetryInterval		= ParseIntValue(OPTION_RETRYINTERVAL, 10);
	m_iControlPort			= ParseIntValue(OPTION_CONTROLPORT, 10);
	m_iSecurePort			= ParseIntValue(OPTION_SECUREPORT, 10);
	m_iUrlConnections		= ParseIntValue(OPTION_URLCONNECTIONS, 10);
	m_iLogBufferSize		= ParseIntValue(OPTION_LOGBUFFERSIZE, 10);
	m_iRotateLog			= ParseIntValue(OPTION_ROTATELOG, 10);
	m_iUMask				= ParseIntValue(OPTION_UMASK, 8);
	m_iUpdateInterval		= ParseIntValue(OPTION_UPDATEINTERVAL, 10);
	m_iWriteBuffer			= ParseIntValue(OPTION_WRITEBUFFER, 10);
	m_iNzbDirInterval		= ParseIntValue(OPTION_NZBDIRINTERVAL, 10);
	m_iNzbDirFileAge		= ParseIntValue(OPTION_NZBDIRFILEAGE, 10);
	m_iDiskSpace			= ParseIntValue(OPTION_DISKSPACE, 10);
	m_iParTimeLimit			= ParseIntValue(OPTION_PARTIMELIMIT, 10);
	m_iKeepHistory			= ParseIntValue(OPTION_KEEPHISTORY, 10);
	m_iFeedHistory			= ParseIntValue(OPTION_FEEDHISTORY, 10);
	m_iTimeCorrection		= ParseIntValue(OPTION_TIMECORRECTION, 10);
	if (-24 <= m_iTimeCorrection && m_iTimeCorrection <= 24)
	{
		m_iTimeCorrection *= 60;
	}
	m_iTimeCorrection *= 60;
	m_iPropagationDelay		= ParseIntValue(OPTION_PROPAGATIONDELAY, 10) * 60;
	m_iArticleCache			= ParseIntValue(OPTION_ARTICLECACHE, 10);
	m_iEventInterval		= ParseIntValue(OPTION_EVENTINTERVAL, 10);
	m_iParBuffer			= ParseIntValue(OPTION_PARBUFFER, 10);
	m_iParThreads			= ParseIntValue(OPTION_PARTHREADS, 10);

	CheckDir(&m_szNzbDir, OPTION_NZBDIR, szMainDir, m_iNzbDirInterval == 0, true);

	m_bBrokenLog			= (bool)ParseEnumValue(OPTION_BROKENLOG, BoolCount, BoolNames, BoolValues);
	m_bNzbLog				= (bool)ParseEnumValue(OPTION_NZBLOG, BoolCount, BoolNames, BoolValues);
	m_bAppendCategoryDir	= (bool)ParseEnumValue(OPTION_APPENDCATEGORYDIR, BoolCount, BoolNames, BoolValues);
	m_bContinuePartial		= (bool)ParseEnumValue(OPTION_CONTINUEPARTIAL, BoolCount, BoolNames, BoolValues);
	m_bSaveQueue			= (bool)ParseEnumValue(OPTION_SAVEQUEUE, BoolCount, BoolNames, BoolValues);
	m_bDupeCheck			= (bool)ParseEnumValue(OPTION_DUPECHECK, BoolCount, BoolNames, BoolValues);
	m_bParRepair			= (bool)ParseEnumValue(OPTION_PARREPAIR, BoolCount, BoolNames, BoolValues);
	m_bParQuick				= (bool)ParseEnumValue(OPTION_PARQUICK, BoolCount, BoolNames, BoolValues);
	m_bParRename			= (bool)ParseEnumValue(OPTION_PARRENAME, BoolCount, BoolNames, BoolValues);
	m_bReloadQueue			= (bool)ParseEnumValue(OPTION_RELOADQUEUE, BoolCount, BoolNames, BoolValues);
	m_bCursesNZBName		= (bool)ParseEnumValue(OPTION_CURSESNZBNAME, BoolCount, BoolNames, BoolValues);
	m_bCursesTime			= (bool)ParseEnumValue(OPTION_CURSESTIME, BoolCount, BoolNames, BoolValues);
	m_bCursesGroup			= (bool)ParseEnumValue(OPTION_CURSESGROUP, BoolCount, BoolNames, BoolValues);
	m_bCrcCheck				= (bool)ParseEnumValue(OPTION_CRCCHECK, BoolCount, BoolNames, BoolValues);
	m_bDirectWrite			= (bool)ParseEnumValue(OPTION_DIRECTWRITE, BoolCount, BoolNames, BoolValues);
	m_bParCleanupQueue		= (bool)ParseEnumValue(OPTION_PARCLEANUPQUEUE, BoolCount, BoolNames, BoolValues);
	m_bDecode				= (bool)ParseEnumValue(OPTION_DECODE, BoolCount, BoolNames, BoolValues);
	m_bDumpCore				= (bool)ParseEnumValue(OPTION_DUMPCORE, BoolCount, BoolNames, BoolValues);
	m_bParPauseQueue		= (bool)ParseEnumValue(OPTION_PARPAUSEQUEUE, BoolCount, BoolNames, BoolValues);
	m_bScriptPauseQueue		= (bool)ParseEnumValue(OPTION_SCRIPTPAUSEQUEUE, BoolCount, BoolNames, BoolValues);
	m_bNzbCleanupDisk		= (bool)ParseEnumValue(OPTION_NZBCLEANUPDISK, BoolCount, BoolNames, BoolValues);
	m_bDeleteCleanupDisk	= (bool)ParseEnumValue(OPTION_DELETECLEANUPDISK, BoolCount, BoolNames, BoolValues);
	m_bAccurateRate			= (bool)ParseEnumValue(OPTION_ACCURATERATE, BoolCount, BoolNames, BoolValues);
	m_bSecureControl		= (bool)ParseEnumValue(OPTION_SECURECONTROL, BoolCount, BoolNames, BoolValues);
	m_bUnpack				= (bool)ParseEnumValue(OPTION_UNPACK, BoolCount, BoolNames, BoolValues);
	m_bUnpackCleanupDisk	= (bool)ParseEnumValue(OPTION_UNPACKCLEANUPDISK, BoolCount, BoolNames, BoolValues);
	m_bUnpackPauseQueue		= (bool)ParseEnumValue(OPTION_UNPACKPAUSEQUEUE, BoolCount, BoolNames, BoolValues);
	m_bUrlForce				= (bool)ParseEnumValue(OPTION_URLFORCE, BoolCount, BoolNames, BoolValues);

	const char* OutputModeNames[] = { "loggable", "logable", "log", "colored", "color", "ncurses", "curses" };
	const int OutputModeValues[] = { omLoggable, omLoggable, omLoggable, omColored, omColored, omNCurses, omNCurses };
	const int OutputModeCount = 7;
	m_eOutputMode = (EOutputMode)ParseEnumValue(OPTION_OUTPUTMODE, OutputModeCount, OutputModeNames, OutputModeValues);

	const char* ParCheckNames[] = { "auto", "always", "force", "manual", "yes", "no" }; // yes/no for compatibility with older versions
	const int ParCheckValues[] = { pcAuto, pcAlways, pcForce, pcManual, pcAlways, pcAuto };
	const int ParCheckCount = 6;
	m_eParCheck = (EParCheck)ParseEnumValue(OPTION_PARCHECK, ParCheckCount, ParCheckNames, ParCheckValues);

	const char* ParScanNames[] = { "limited", "full", "auto" };
	const int ParScanValues[] = { psLimited, psFull, psAuto };
	const int ParScanCount = 3;
	m_eParScan = (EParScan)ParseEnumValue(OPTION_PARSCAN, ParScanCount, ParScanNames, ParScanValues);

	const char* HealthCheckNames[] = { "pause", "delete", "none" };
	const int HealthCheckValues[] = { hcPause, hcDelete, hcNone };
	const int HealthCheckCount = 3;
	m_eHealthCheck = (EHealthCheck)ParseEnumValue(OPTION_HEALTHCHECK, HealthCheckCount, HealthCheckNames, HealthCheckValues);

	const char* TargetNames[] = { "screen", "log", "both", "none" };
	const int TargetValues[] = { mtScreen, mtLog, mtBoth, mtNone };
	const int TargetCount = 4;
	m_eInfoTarget = (EMessageTarget)ParseEnumValue(OPTION_INFOTARGET, TargetCount, TargetNames, TargetValues);
	m_eWarningTarget = (EMessageTarget)ParseEnumValue(OPTION_WARNINGTARGET, TargetCount, TargetNames, TargetValues);
	m_eErrorTarget = (EMessageTarget)ParseEnumValue(OPTION_ERRORTARGET, TargetCount, TargetNames, TargetValues);
	m_eDebugTarget = (EMessageTarget)ParseEnumValue(OPTION_DEBUGTARGET, TargetCount, TargetNames, TargetValues);
	m_eDetailTarget = (EMessageTarget)ParseEnumValue(OPTION_DETAILTARGET, TargetCount, TargetNames, TargetValues);

	const char* WriteLogNames[] = { "none", "append", "reset", "rotate" };
	const int WriteLogValues[] = { wlNone, wlAppend, wlReset, wlRotate };
	const int WriteLogCount = 4;
	m_eWriteLog = (EWriteLog)ParseEnumValue(OPTION_WRITELOG, WriteLogCount, WriteLogNames, WriteLogValues);
}

int Options::ParseEnumValue(const char* OptName, int argc, const char * argn[], const int argv[])
{
	OptEntry* pOptEntry = FindOption(OptName);
	if (!pOptEntry)
	{
		ConfigError("Undefined value for option \"%s\"", OptName);
		return argv[0];
	}

	int iDefNum = 0;

	for (int i = 0; i < argc; i++)
	{
		if (!strcasecmp(pOptEntry->GetValue(), argn[i]))
		{
			// normalizing option value in option list, for example "NO" -> "no"
			for (int j = 0; j < argc; j++)
			{
				if (argv[j] == argv[i])
				{
					if (strcmp(argn[j], pOptEntry->GetValue()))
					{
						pOptEntry->SetValue(argn[j]);
					}
					break;
				}
			}

			return argv[i];
		}

		if (!strcasecmp(pOptEntry->GetDefValue(), argn[i]))
		{
			iDefNum = i;
		}
	}

	m_iConfigLine = pOptEntry->GetLineNo();
	ConfigError("Invalid value for option \"%s\": \"%s\"", OptName, pOptEntry->GetValue());
	pOptEntry->SetValue(argn[iDefNum]);
	return argv[iDefNum];
}

int Options::ParseIntValue(const char* OptName, int iBase)
{
	OptEntry* pOptEntry = FindOption(OptName);
	if (!pOptEntry)
	{
		abort("FATAL ERROR: Undefined value for option \"%s\"\n", OptName);
	}

	char *endptr;
	int val = strtol(pOptEntry->GetValue(), &endptr, iBase);

	if (endptr && *endptr != '\0')
	{
		m_iConfigLine = pOptEntry->GetLineNo();
		ConfigError("Invalid value for option \"%s\": \"%s\"", OptName, pOptEntry->GetValue());
		pOptEntry->SetValue(pOptEntry->GetDefValue());
		val = strtol(pOptEntry->GetDefValue(), NULL, iBase);
	}

	return val;
}

float Options::ParseFloatValue(const char* OptName)
{
	OptEntry* pOptEntry = FindOption(OptName);
	if (!pOptEntry)
	{
		abort("FATAL ERROR: Undefined value for option \"%s\"\n", OptName);
	}

	char *endptr;
	float val = (float)strtod(pOptEntry->GetValue(), &endptr);

	if (endptr && *endptr != '\0')
	{
		m_iConfigLine = pOptEntry->GetLineNo();
		ConfigError("Invalid value for option \"%s\": \"%s\"", OptName, pOptEntry->GetValue());
		pOptEntry->SetValue(pOptEntry->GetDefValue());
		val = (float)strtod(pOptEntry->GetDefValue(), NULL);
	}

	return val;
}

void Options::InitCommandLine(int argc, char* argv[])
{
	m_eClientOperation = opClientNoOperation; // default

	// reset getopt
	optind = 1;

	while (true)
	{
		int c;

#ifdef HAVE_GETOPT_LONG
		int option_index  = 0;
		c = getopt_long(argc, argv, short_options, long_options, &option_index);
#else
		c = getopt(argc, argv, short_options);
#endif

		if (c == -1) break;

		switch (c)
		{
			case 'c':
				m_szConfigFilename = strdup(optarg);
				break;
			case 'n':
				m_szConfigFilename = NULL;
				m_bNoConfig = true;
				break;
			case 'h':
				PrintUsage(argv[0]);
				exit(0);
				break;
			case 'v':
				printf("nzbget version: %s\n", Util::VersionRevision());
				exit(1);
				break;
			case 'p':
				m_bPrintOptions = true;
				break;
			case 'o':
				InitOptFile();
				if (!SetOptionString(optarg))
				{
					abort("FATAL ERROR: error in option \"%s\"\n", optarg);
				}
				break;
			case 's':
				m_bServerMode = true;
				break;
			case 'D':
				m_bServerMode = true;
				m_bDaemonMode = true;
				break;
			case 'A':
				m_eClientOperation = opClientRequestDownload;

				while (true)
				{
					optind++;
					optarg = optind > argc ? NULL : argv[optind-1];
					if (optarg && (!strcasecmp(optarg, "F") || !strcasecmp(optarg, "U")))
					{
						// option ignored (but kept for compatibility)
					}
					else if (optarg && !strcasecmp(optarg, "T"))
					{
						m_bAddTop = true;
					}
					else if (optarg && !strcasecmp(optarg, "P"))
					{
						m_bAddPaused = true;
					}
					else if (optarg && !strcasecmp(optarg, "I"))
					{
						optind++;
						if (optind > argc)
						{
							abort("FATAL ERROR: Could not parse value of option 'A'\n");
						}
						m_iAddPriority = atoi(argv[optind-1]);
					}
					else if (optarg && !strcasecmp(optarg, "C"))
					{
						optind++;
						if (optind > argc)
						{
							abort("FATAL ERROR: Could not parse value of option 'A'\n");
						}
						free(m_szAddCategory);
						m_szAddCategory = strdup(argv[optind-1]);
					}
					else if (optarg && !strcasecmp(optarg, "N"))
					{
						optind++;
						if (optind > argc)
						{
							abort("FATAL ERROR: Could not parse value of option 'A'\n");
						}
						free(m_szAddNZBFilename);
						m_szAddNZBFilename = strdup(argv[optind-1]);
					}
					else if (optarg && !strcasecmp(optarg, "DK"))
					{
						optind++;
						if (optind > argc)
						{
							abort("FATAL ERROR: Could not parse value of option 'A'\n");
						}
						free(m_szAddDupeKey);
						m_szAddDupeKey = strdup(argv[optind-1]);
					}
					else if (optarg && !strcasecmp(optarg, "DS"))
					{
						optind++;
						if (optind > argc)
						{
							abort("FATAL ERROR: Could not parse value of option 'A'\n");
						}
						m_iAddDupeScore = atoi(argv[optind-1]);
					}
					else if (optarg && !strcasecmp(optarg, "DM"))
					{
						optind++;
						if (optind > argc)
						{
							abort("FATAL ERROR: Could not parse value of option 'A'\n");
						}

						const char* szDupeMode = argv[optind-1];
						if (!strcasecmp(szDupeMode, "score"))
						{
							m_iAddDupeMode = dmScore;
						}
						else if (!strcasecmp(szDupeMode, "all"))
						{
							m_iAddDupeMode = dmAll;
						}
						else if (!strcasecmp(szDupeMode, "force"))
						{
							m_iAddDupeMode = dmForce;
						}
						else
						{
							abort("FATAL ERROR: Could not parse value of option 'A'\n");
						}
					}
					else
					{
						optind--;
						break;
					}
				}
				break;
			case 'L':
				optind++;
				optarg = optind > argc ? NULL : argv[optind-1];
				if (!optarg || !strncmp(optarg, "-", 1))
				{
					m_eClientOperation = opClientRequestListFiles;
					optind--;
				}
				else if (!strcasecmp(optarg, "F") || !strcasecmp(optarg, "FR"))
				{
					m_eClientOperation = opClientRequestListFiles;
				}
				else if (!strcasecmp(optarg, "G") || !strcasecmp(optarg, "GR"))
				{
					m_eClientOperation = opClientRequestListGroups;
				}
				else if (!strcasecmp(optarg, "O"))
				{
					m_eClientOperation = opClientRequestPostQueue;
				}
				else if (!strcasecmp(optarg, "S"))
				{
					m_eClientOperation = opClientRequestListStatus;
				}
				else if (!strcasecmp(optarg, "H"))
				{
					m_eClientOperation = opClientRequestHistory;
				}
				else if (!strcasecmp(optarg, "HA"))
				{
					m_eClientOperation = opClientRequestHistoryAll;
				}
				else
				{
					abort("FATAL ERROR: Could not parse value of option 'L'\n");
				}

				if (optarg && (!strcasecmp(optarg, "FR") || !strcasecmp(optarg, "GR")))
				{
					m_EMatchMode = mmRegEx;

					optind++;
					if (optind > argc)
					{
						abort("FATAL ERROR: Could not parse value of option 'L'\n");
					}
					m_szEditQueueText = strdup(argv[optind-1]);
				}
				break;
			case 'P':
			case 'U':
				optind++;
				optarg = optind > argc ? NULL : argv[optind-1];
				if (!optarg || !strncmp(optarg, "-", 1))
				{
					m_eClientOperation = c == 'P' ? opClientRequestDownloadPause : opClientRequestDownloadUnpause;
					optind--;
				}
				else if (!strcasecmp(optarg, "D"))
				{
					m_eClientOperation = c == 'P' ? opClientRequestDownloadPause : opClientRequestDownloadUnpause;
				}
				else if (!strcasecmp(optarg, "O"))
				{
					m_eClientOperation = c == 'P' ? opClientRequestPostPause : opClientRequestPostUnpause;
				}
				else if (!strcasecmp(optarg, "S"))
				{
					m_eClientOperation = c == 'P' ? opClientRequestScanPause : opClientRequestScanUnpause;
				}
				else
				{
					abort("FATAL ERROR: Could not parse value of option '%c'\n", c);
				}
				break;
			case 'R':
				m_eClientOperation = opClientRequestSetRate;
				m_iSetRate = (int)(atof(optarg)*1024);
				break;
			case 'B':
				if (!strcasecmp(optarg, "dump"))
				{
					m_eClientOperation = opClientRequestDumpDebug;
				}
				else if (!strcasecmp(optarg, "trace"))
				{
					m_bTestBacktrace = true;
				}
				else if (!strcasecmp(optarg, "webget"))
				{
					m_bWebGet = true;
					optind++;
					if (optind > argc)
					{
						abort("FATAL ERROR: Could not parse value of option 'E'\n");
					}
					optarg = argv[optind-1];
					m_szWebGetFilename = strdup(optarg);
				}
				else
				{
					abort("FATAL ERROR: Could not parse value of option 'B'\n");
				}
				break;
			case 'G':
				m_eClientOperation = opClientRequestLog;
				m_iLogLines = atoi(optarg);
				if (m_iLogLines == 0)
				{
					abort("FATAL ERROR: Could not parse value of option 'G'\n");
				}
				break;
			case 'T':
				m_bAddTop = true;
				break;
			case 'C':
				m_bRemoteClientMode = true;
				break;
			case 'E':
			{
				m_eClientOperation = opClientRequestEditQueue;
				bool bGroup = !strcasecmp(optarg, "G") || !strcasecmp(optarg, "GN") || !strcasecmp(optarg, "GR");
				bool bFile = !strcasecmp(optarg, "F") || !strcasecmp(optarg, "FN") || !strcasecmp(optarg, "FR");
				if (!strcasecmp(optarg, "GN") || !strcasecmp(optarg, "FN"))
				{
					m_EMatchMode = mmName;
				}
				else if (!strcasecmp(optarg, "GR") || !strcasecmp(optarg, "FR"))
				{
					m_EMatchMode = mmRegEx;
				}
				else
				{
					m_EMatchMode = mmID;
				};
				bool bPost = !strcasecmp(optarg, "O");
				bool bHistory = !strcasecmp(optarg, "H");
				if (bGroup || bFile || bPost || bHistory)
				{
					optind++;
					if (optind > argc)
					{
						abort("FATAL ERROR: Could not parse value of option 'E'\n");
					}
					optarg = argv[optind-1];
				}

				if (bPost)
				{
					// edit-commands for post-processor-queue
					if (!strcasecmp(optarg, "D"))
					{
						m_iEditQueueAction = DownloadQueue::eaPostDelete;
					}
					else
					{
						abort("FATAL ERROR: Could not parse value of option 'E'\n");
					}
				}
				else if (bHistory)
				{
					// edit-commands for history
					if (!strcasecmp(optarg, "D"))
					{
						m_iEditQueueAction = DownloadQueue::eaHistoryDelete;
					}
					else if (!strcasecmp(optarg, "R"))
					{
						m_iEditQueueAction = DownloadQueue::eaHistoryReturn;
					}
					else if (!strcasecmp(optarg, "P"))
					{
						m_iEditQueueAction = DownloadQueue::eaHistoryProcess;
					}
					else if (!strcasecmp(optarg, "A"))
					{
						m_iEditQueueAction = DownloadQueue::eaHistoryRedownload;
					}
					else if (!strcasecmp(optarg, "O"))
					{
						m_iEditQueueAction = DownloadQueue::eaHistorySetParameter;
						
						optind++;
						if (optind > argc)
						{
							abort("FATAL ERROR: Could not parse value of option 'E'\n");
						}
						m_szEditQueueText = strdup(argv[optind-1]);
						
						if (!strchr(m_szEditQueueText, '='))
						{
							abort("FATAL ERROR: Could not parse value of option 'E'\n");
						}
					}
					else if (!strcasecmp(optarg, "B"))
					{
						m_iEditQueueAction = DownloadQueue::eaHistoryMarkBad;
					}
					else if (!strcasecmp(optarg, "G"))
					{
						m_iEditQueueAction = DownloadQueue::eaHistoryMarkGood;
					}
					else if (!strcasecmp(optarg, "S"))
					{
						m_iEditQueueAction = DownloadQueue::eaHistoryMarkSuccess;
					}
					else
					{
						abort("FATAL ERROR: Could not parse value of option 'E'\n");
					}
				}
				else
				{
					// edit-commands for download-queue
					if (!strcasecmp(optarg, "T"))
					{
						m_iEditQueueAction = bGroup ? DownloadQueue::eaGroupMoveTop : DownloadQueue::eaFileMoveTop;
					}
					else if (!strcasecmp(optarg, "B"))
					{
						m_iEditQueueAction = bGroup ? DownloadQueue::eaGroupMoveBottom : DownloadQueue::eaFileMoveBottom;
					}
					else if (!strcasecmp(optarg, "P"))
					{
						m_iEditQueueAction = bGroup ? DownloadQueue::eaGroupPause : DownloadQueue::eaFilePause;
					}
					else if (!strcasecmp(optarg, "A"))
					{
						m_iEditQueueAction = bGroup ? DownloadQueue::eaGroupPauseAllPars : DownloadQueue::eaFilePauseAllPars;
					}
					else if (!strcasecmp(optarg, "R"))
					{
						m_iEditQueueAction = bGroup ? DownloadQueue::eaGroupPauseExtraPars : DownloadQueue::eaFilePauseExtraPars;
					}
					else if (!strcasecmp(optarg, "U"))
					{
						m_iEditQueueAction = bGroup ? DownloadQueue::eaGroupResume : DownloadQueue::eaFileResume;
					}
					else if (!strcasecmp(optarg, "D"))
					{
						m_iEditQueueAction = bGroup ? DownloadQueue::eaGroupDelete : DownloadQueue::eaFileDelete;
					}
					else if (!strcasecmp(optarg, "C") || !strcasecmp(optarg, "K") || !strcasecmp(optarg, "CP"))
					{
						// switch "K" is provided for compatibility with v. 0.8.0 and can be removed in future versions
						if (!bGroup)
						{
							abort("FATAL ERROR: Category can be set only for groups\n");
						}
						m_iEditQueueAction = !strcasecmp(optarg, "CP") ? DownloadQueue::eaGroupApplyCategory : DownloadQueue::eaGroupSetCategory;

						optind++;
						if (optind > argc)
						{
							abort("FATAL ERROR: Could not parse value of option 'E'\n");
						}
						m_szEditQueueText = strdup(argv[optind-1]);
					}
					else if (!strcasecmp(optarg, "N"))
					{
						if (!bGroup)
						{
							abort("FATAL ERROR: Only groups can be renamed\n");
						}
						m_iEditQueueAction = DownloadQueue::eaGroupSetName;

						optind++;
						if (optind > argc)
						{
							abort("FATAL ERROR: Could not parse value of option 'E'\n");
						}
						m_szEditQueueText = strdup(argv[optind-1]);
					}
					else if (!strcasecmp(optarg, "M"))
					{
						if (!bGroup)
						{
							abort("FATAL ERROR: Only groups can be merged\n");
						}
						m_iEditQueueAction = DownloadQueue::eaGroupMerge;
					}
					else if (!strcasecmp(optarg, "S"))
					{
						m_iEditQueueAction = DownloadQueue::eaFileSplit;

						optind++;
						if (optind > argc)
						{
							abort("FATAL ERROR: Could not parse value of option 'E'\n");
						}
						m_szEditQueueText = strdup(argv[optind-1]);
					}
					else if (!strcasecmp(optarg, "O"))
					{
						if (!bGroup)
						{
							abort("FATAL ERROR: Post-process parameter can be set only for groups\n");
						}
						m_iEditQueueAction = DownloadQueue::eaGroupSetParameter;

						optind++;
						if (optind > argc)
						{
							abort("FATAL ERROR: Could not parse value of option 'E'\n");
						}
						m_szEditQueueText = strdup(argv[optind-1]);

						if (!strchr(m_szEditQueueText, '='))
						{
							abort("FATAL ERROR: Could not parse value of option 'E'\n");
						}
					}
					else if (!strcasecmp(optarg, "I"))
					{
						if (!bGroup)
						{
							abort("FATAL ERROR: Priority can be set only for groups\n");
						}
						m_iEditQueueAction = DownloadQueue::eaGroupSetPriority;

						optind++;
						if (optind > argc)
						{
							abort("FATAL ERROR: Could not parse value of option 'E'\n");
						}
						m_szEditQueueText = strdup(argv[optind-1]);

						if (atoi(m_szEditQueueText) == 0 && strcmp("0", m_szEditQueueText))
						{
							abort("FATAL ERROR: Could not parse value of option 'E'\n");
						}
					}
					else
					{
						m_iEditQueueOffset = atoi(optarg);
						if (m_iEditQueueOffset == 0)
						{
							abort("FATAL ERROR: Could not parse value of option 'E'\n");
						}
						m_iEditQueueAction = bGroup ? DownloadQueue::eaGroupMoveOffset : DownloadQueue::eaFileMoveOffset;
					}
				}
				break;
			}
			case 'Q':
				m_eClientOperation = opClientRequestShutdown;
				break;
			case 'O':
				m_eClientOperation = opClientRequestReload;
				break;
			case 'V':
				m_eClientOperation = opClientRequestVersion;
				break;
			case 'W':
				m_eClientOperation = opClientRequestWriteLog;
				if (!strcasecmp(optarg, "I")) {
					m_iWriteLogKind = (int)Message::mkInfo;
				}
				else if (!strcasecmp(optarg, "W")) {
					m_iWriteLogKind = (int)Message::mkWarning;
				}
				else if (!strcasecmp(optarg, "E")) {
					m_iWriteLogKind = (int)Message::mkError;
				}
				else if (!strcasecmp(optarg, "D")) {
					m_iWriteLogKind = (int)Message::mkDetail;
				}
				else if (!strcasecmp(optarg, "G")) {
					m_iWriteLogKind = (int)Message::mkDebug;
				}
				else
				{
					abort("FATAL ERROR: Could not parse value of option 'W'\n");
				}
				break;
			case 'K':
				// switch "K" is provided for compatibility with v. 0.8.0 and can be removed in future versions
				free(m_szAddCategory);
				m_szAddCategory = strdup(optarg);
				break;
			case 'S':
				optind++;
				optarg = optind > argc ? NULL : argv[optind-1];
				if (!optarg || !strncmp(optarg, "-", 1))
				{
					m_eClientOperation = opClientRequestScanAsync;
					optind--;
				}
				else if (!strcasecmp(optarg, "W"))
				{
					m_eClientOperation = opClientRequestScanSync;
				}
				else
				{
					abort("FATAL ERROR: Could not parse value of option '%c'\n", c);
				}
				break;
			case '?':
				exit(-1);
				break;
		}
	}

	if (m_bServerMode && m_eClientOperation == opClientRequestDownloadPause)
	{
		m_bPauseDownload = true;
		m_eClientOperation = opClientNoOperation;
	}

	InitOptFile();
}

void Options::PrintUsage(char* com)
{
	printf("Usage:\n"
		"  %s [switches]\n\n"
		"Switches:\n"
	    "  -h, --help                Print this help-message\n"
	    "  -v, --version             Print version and exit\n"
		"  -c, --configfile <file>   Filename of configuration-file\n"
		"  -n, --noconfigfile        Prevent loading of configuration-file\n"
		"                            (required options must be passed with --option)\n"
		"  -p, --printconfig         Print configuration and exit\n"
		"  -o, --option <name=value> Set or override option in configuration-file\n"
		"  -s, --server              Start nzbget as a server in console-mode\n"
#ifndef WIN32
		"  -D, --daemon              Start nzbget as a server in daemon-mode\n"
#endif
	    "  -V, --serverversion       Print server's version and exit\n"
		"  -Q, --quit                Shutdown server\n"
		"  -O, --reload              Reload config and restart all services\n"
		"  -A, --append [<options>] <nzb-file/url> Send file/url to server's\n"
		"                            download queue\n"
		"    <options> are (multiple options must be separated with space):\n"
		"       T                    Add file to the top (beginning) of queue\n"
		"       P                    Pause added files\n"
		"       C <name>             Assign category to nzb-file\n"
		"       N <name>             Use this name as nzb-filename\n"
		"       I <priority>         Set priority (signed integer)\n"
		"       DK <dupekey>         Set duplicate key (string)\n"
		"       DS <dupescore>       Set duplicate score (signed integer)\n"
		"       DM (score|all|force) Set duplicate mode\n"
		"  -C, --connect             Attach client to server\n"
		"  -L, --list    [F|FR|G|GR|O|H|S] [RegEx] Request list of items from server\n"
		"                 F          List individual files and server status (default)\n"
		"                 FR         Like \"F\" but apply regular expression filter\n"
		"                 G          List groups (nzb-files) and server status\n"
		"                 GR         Like \"G\" but apply regular expression filter\n"
		"                 O          List post-processor-queue\n"
		"                 H          List history\n"
		"                 HA         List history, all records (incl. hidden)\n"
		"                 S          Print only server status\n"
		"    <RegEx>                 Regular expression (only with options \"FR\", \"GR\")\n"
		"                            using POSIX Extended Regular Expression Syntax\n"
		"  -P, --pause   [D|O|S]  Pause server\n"
		"                 D          Pause download queue (default)\n"
		"                 O          Pause post-processor queue\n"
		"                 S          Pause scan of incoming nzb-directory\n"
		"  -U, --unpause [D|O|S]  Unpause server\n"
		"                 D          Unpause download queue (default)\n"
		"                 O          Unpause post-processor queue\n"
		"                 S          Unpause scan of incoming nzb-directory\n"
		"  -R, --rate <speed>        Set download rate on server, in KB/s\n"
		"  -G, --log <lines>         Request last <lines> lines from server's screen-log\n"
		"  -W, --write <D|I|W|E|G> \"Text\" Send text to server's log\n"
		"  -S, --scan    [W]         Scan incoming nzb-directory on server\n"
		"                 W          Wait until scan completes (synchronous mode)\n"
		"  -E, --edit [F|FN|FR|G|GN|GR|O|H] <action> <IDs/Names/RegExs> Edit items\n"
		"                            on server\n"
		"              F             Edit individual files (default)\n"
		"              FN            Like \"F\" but uses names (as \"group/file\")\n"
		"                            instead of IDs\n"
		"              FR            Like \"FN\" but with regular expressions\n"
		"              G             Edit all files in the group (same nzb-file)\n"
		"              GN            Like \"G\" but uses group names instead of IDs\n"
		"              GR            Like \"GN\" but with regular expressions\n"
		"              O             Edit post-processor-queue\n"
		"              H             Edit history\n"
		"    <action> is one of:\n"
		"    - for files (F) and groups (G):\n"
		"       <+offset|-offset>    Move in queue relative to current position,\n"
		"                            offset is an integer value\n"
		"       T                    Move to top of queue\n"
		"       B                    Move to bottom of queue\n"
		"       D                    Delete\n"
		"    - for files (F) and groups (G):\n"
		"       P                    Pause\n"
		"       U                    Resume (unpause)\n"
		"    - for groups (G):\n"
		"       A                    Pause all pars\n"
		"       R                    Pause extra pars\n"
		"       I <priority>         Set priority (signed integer)\n"
		"       C <name>             Set category\n"
		"       CP <name>            Set category and apply post-process parameters\n"
		"       N <name>             Rename\n"
		"       M                    Merge\n"
		"       S <name>             Split - create new group from selected files\n"
		"       O <name>=<value>     Set post-process parameter\n"
		"    - for post-jobs (O):\n"
		"       D                    Delete (cancel post-processing)\n"
		"    - for history (H):\n"
		"       D                    Delete\n"
		"       P                    Post-process again\n"
		"       R                    Download remaining files\n"
		"       A                    Download again\n"
		"       O <name>=<value>     Set post-process parameter\n"
		"       B                    Mark as bad\n"
		"       G                    Mark as good\n"
		"       S                    Mark as success\n"
		"    <IDs>                   Comma-separated list of file- or group- ids or\n"
		"                            ranges of file-ids, e. g.: 1-5,3,10-22\n"
		"    <Names>                 List of names (with options \"FN\" and \"GN\"),\n"
		"                            e. g.: \"my nzb download%cmyfile.nfo\" \"another nzb\"\n"
		"    <RegExs>                List of regular expressions (options \"FR\", \"GR\")\n"
		"                            using POSIX Extended Regular Expression Syntax\n",
		Util::BaseFileName(com),
		PATH_SEPARATOR);
}

void Options::InitFileArg(int argc, char* argv[])
{
	if (optind >= argc)
	{
		// no nzb-file passed
		if (!m_bServerMode && !m_bRemoteClientMode &&
		        (m_eClientOperation == opClientNoOperation ||
		         m_eClientOperation == opClientRequestDownload ||
				 m_eClientOperation == opClientRequestWriteLog))
		{
			if (m_eClientOperation == opClientRequestWriteLog)
			{
				abort("FATAL ERROR: Log-text not specified\n");
			}
			else
			{
				abort("FATAL ERROR: Nzb-file or Url not specified\n");
			}
		}
	}
	else if (m_eClientOperation == opClientRequestEditQueue)
	{
		if (m_EMatchMode == mmID)
		{
			ParseFileIDList(argc, argv, optind);
		}
		else
		{
			ParseFileNameList(argc, argv, optind);
		}
	}
	else
	{
		m_szLastArg = strdup(argv[optind]);

		// Check if the file-name is a relative path or an absolute path
		// If the path starts with '/' its an absolute, else relative
		const char* szFileName = argv[optind];

#ifdef WIN32
			m_szArgFilename = strdup(szFileName);
#else
		if (szFileName[0] == '/' || !strncasecmp(szFileName, "http://", 6) || !strncasecmp(szFileName, "https://", 7))
		{
			m_szArgFilename = strdup(szFileName);
		}
		else
		{
			// TEST
			char szFileNameWithPath[1024];
			getcwd(szFileNameWithPath, 1024);
			strcat(szFileNameWithPath, "/");
			strcat(szFileNameWithPath, szFileName);
			m_szArgFilename = strdup(szFileNameWithPath);
		}
#endif

		if (m_bServerMode || m_bRemoteClientMode ||
		        !(m_eClientOperation == opClientNoOperation ||
		          m_eClientOperation == opClientRequestDownload ||
				  m_eClientOperation == opClientRequestWriteLog))
		{
			abort("FATAL ERROR: Too many arguments\n");
		}
	}
}

const char* Options::GetControlIP()
{
	if ((m_bRemoteClientMode || m_eClientOperation != opClientNoOperation) &&
		!strcmp(m_szControlIP, "0.0.0.0"))
	{
		return "127.0.0.1";
	}
	return m_szControlIP;
}

void Options::SetOption(const char* optname, const char* value)
{
	OptEntry* pOptEntry = FindOption(optname);
	if (!pOptEntry)
	{
		pOptEntry = new OptEntry();
		pOptEntry->SetName(optname);
		m_OptEntries.push_back(pOptEntry);
	}

	char* curvalue = NULL;

#ifndef WIN32
	if (value && (value[0] == '~') && (value[1] == '/'))
	{
		char szExpandedPath[1024];
		if (!Util::ExpandHomePath(value, szExpandedPath, sizeof(szExpandedPath)))
		{
			ConfigError("Invalid value for option\"%s\": unable to determine home-directory", optname);
			szExpandedPath[0] = '\0';
		}
		curvalue = strdup(szExpandedPath);
	}
	else
#endif
	{
		curvalue = strdup(value);
	}

	pOptEntry->SetLineNo(m_iConfigLine);

	// expand variables
	while (char* dollar = strstr(curvalue, "${"))
	{
		char* end = strchr(dollar, '}');
		if (end)
		{
			int varlen = (int)(end - dollar - 2);
			char variable[101];
			int maxlen = varlen < 100 ? varlen : 100;
			strncpy(variable, dollar + 2, maxlen);
			variable[maxlen] = '\0';
			const char* varvalue = GetOption(variable);
			if (varvalue)
			{
				int newlen = strlen(varvalue);
				char* newvalue = (char*)malloc(strlen(curvalue) - varlen - 3 + newlen + 1);
				strncpy(newvalue, curvalue, dollar - curvalue);
				strncpy(newvalue + (dollar - curvalue), varvalue, newlen);
				strcpy(newvalue + (dollar - curvalue) + newlen, end + 1);
				free(curvalue);
				curvalue = newvalue;
			}
			else
			{
				break;
			}
		}
		else
		{
			break;
		}
	}

	pOptEntry->SetValue(curvalue);

	free(curvalue);
}

Options::OptEntry* Options::FindOption(const char* optname)
{
	OptEntry* pOptEntry = m_OptEntries.FindOption(optname);

	// normalize option name in option list; for example "server1.joingroup" -> "Server1.JoinGroup"
	if (pOptEntry && strcmp(pOptEntry->GetName(), optname))
	{
		pOptEntry->SetName(optname);
	}

	return pOptEntry;
}

const char* Options::GetOption(const char* optname)
{
	OptEntry* pOptEntry = FindOption(optname);
	if (pOptEntry)
	{
		if (pOptEntry->GetLineNo() > 0)
		{
			m_iConfigLine = pOptEntry->GetLineNo();
		}
		return pOptEntry->GetValue();
	}
	return NULL;
}

void Options::InitServers()
{
	int n = 1;
	while (true)
	{
		char optname[128];

		sprintf(optname, "Server%i.Active", n);
		const char* nactive = GetOption(optname);
		bool bActive = true;
		if (nactive)
		{
			bActive = (bool)ParseEnumValue(optname, BoolCount, BoolNames, BoolValues);
		}

		sprintf(optname, "Server%i.Name", n);
		const char* nname = GetOption(optname);

		sprintf(optname, "Server%i.Level", n);
		const char* nlevel = GetOption(optname);

		sprintf(optname, "Server%i.Group", n);
		const char* ngroup = GetOption(optname);
		
		sprintf(optname, "Server%i.Host", n);
		const char* nhost = GetOption(optname);

		sprintf(optname, "Server%i.Port", n);
		const char* nport = GetOption(optname);

		sprintf(optname, "Server%i.Username", n);
		const char* nusername = GetOption(optname);

		sprintf(optname, "Server%i.Password", n);
		const char* npassword = GetOption(optname);

		sprintf(optname, "Server%i.JoinGroup", n);
		const char* njoingroup = GetOption(optname);
		bool bJoinGroup = false;
		if (njoingroup)
		{
			bJoinGroup = (bool)ParseEnumValue(optname, BoolCount, BoolNames, BoolValues);
		}

		sprintf(optname, "Server%i.Encryption", n);
		const char* ntls = GetOption(optname);
		bool bTLS = false;
		if (ntls)
		{
			bTLS = (bool)ParseEnumValue(optname, BoolCount, BoolNames, BoolValues);
#ifdef DISABLE_TLS
			if (bTLS)
			{
				ConfigError("Invalid value for option \"%s\": program was compiled without TLS/SSL-support", optname);
				bTLS = false;
			}
#endif
			m_bTLS |= bTLS;
		}

		sprintf(optname, "Server%i.Cipher", n);
		const char* ncipher = GetOption(optname);

		sprintf(optname, "Server%i.Connections", n);
		const char* nconnections = GetOption(optname);

		sprintf(optname, "Server%i.Retention", n);
		const char* nretention = GetOption(optname);

		bool definition = nactive || nname || nlevel || ngroup || nhost || nport ||
			nusername || npassword || nconnections || njoingroup || ntls || ncipher || nretention;
		bool completed = nhost && nport && nconnections;

		if (!definition)
		{
			break;
		}

		if (completed)
		{
			NewsServer* pNewsServer = new NewsServer(n, bActive, nname,
				nhost,
				nport ? atoi(nport) : 119,
				nusername, npassword,
				bJoinGroup, bTLS, ncipher,
				nconnections ? atoi(nconnections) : 1,
				nretention ? atoi(nretention) : 0,
				nlevel ? atoi(nlevel) : 0,
				ngroup ? atoi(ngroup) : 0);
			g_pServerPool->AddServer(pNewsServer);
		}
		else
		{
			ConfigError("Server definition not complete for \"Server%i\"", n);
		}

		n++;
	}

	g_pServerPool->SetTimeout(GetArticleTimeout());
	g_pServerPool->SetRetryInterval(GetRetryInterval());
}

void Options::InitCategories()
{
	int n = 1;
	while (true)
	{
		char optname[128];

		sprintf(optname, "Category%i.Name", n);
		const char* nname = GetOption(optname);

		char destdiroptname[128];
		sprintf(destdiroptname, "Category%i.DestDir", n);
		const char* ndestdir = GetOption(destdiroptname);

		sprintf(optname, "Category%i.Unpack", n);
		const char* nunpack = GetOption(optname);
		bool bUnpack = true;
		if (nunpack)
		{
			bUnpack = (bool)ParseEnumValue(optname, BoolCount, BoolNames, BoolValues);
		}

		sprintf(optname, "Category%i.PostScript", n);
		const char* npostscript = GetOption(optname);

		sprintf(optname, "Category%i.Aliases", n);
		const char* naliases = GetOption(optname);

		bool definition = nname || ndestdir || nunpack || npostscript || naliases;
		bool completed = nname && strlen(nname) > 0;

		if (!definition)
		{
			break;
		}

		if (completed)
		{
			char* szDestDir = NULL;
			if (ndestdir && ndestdir[0] != '\0')
			{
				CheckDir(&szDestDir, destdiroptname, m_szDestDir, false, false);
			}

			Category* pCategory = new Category(nname, szDestDir, bUnpack, npostscript);
			m_Categories.push_back(pCategory);

			free(szDestDir);
			
			// split Aliases into tokens and create items for each token
			if (naliases)
			{
				Tokenizer tok(naliases, ",;");
				while (const char* szAliasName = tok.Next())
				{
					pCategory->GetAliases()->push_back(strdup(szAliasName));
				}
			}
		}
		else
		{
			ConfigError("Category definition not complete for \"Category%i\"", n);
		}

		n++;
	}
}

void Options::InitFeeds()
{
	int n = 1;
	while (true)
	{
		char optname[128];

		sprintf(optname, "Feed%i.Name", n);
		const char* nname = GetOption(optname);

		sprintf(optname, "Feed%i.URL", n);
		const char* nurl = GetOption(optname);
		
		sprintf(optname, "Feed%i.Filter", n);
		const char* nfilter = GetOption(optname);

		sprintf(optname, "Feed%i.Category", n);
		const char* ncategory = GetOption(optname);

		sprintf(optname, "Feed%i.PauseNzb", n);
		const char* npausenzb = GetOption(optname);
		bool bPauseNzb = false;
		if (npausenzb)
		{
			bPauseNzb = (bool)ParseEnumValue(optname, BoolCount, BoolNames, BoolValues);
		}

		sprintf(optname, "Feed%i.Interval", n);
		const char* ninterval = GetOption(optname);

		sprintf(optname, "Feed%i.Priority", n);
		const char* npriority = GetOption(optname);

		bool definition = nname || nurl || nfilter || ncategory || npausenzb || ninterval || npriority;
		bool completed = nurl;

		if (!definition)
		{
			break;
		}

		if (completed)
		{
			FeedInfo* pFeedInfo = new FeedInfo(n, nname, nurl, ninterval ? atoi(ninterval) : 0, nfilter, 
				bPauseNzb, ncategory, npriority ? atoi(npriority) : 0);
			g_pFeedCoordinator->AddFeed(pFeedInfo);
		}
		else
		{
			ConfigError("Feed definition not complete for \"Feed%i\"", n);
		}

		n++;
	}
}

void Options::InitScheduler()
{
	for (int n = 1; ; n++)
	{
		char optname[128];

		sprintf(optname, "Task%i.Time", n);
		const char* szTime = GetOption(optname);

		sprintf(optname, "Task%i.WeekDays", n);
		const char* szWeekDays = GetOption(optname);

		sprintf(optname, "Task%i.Command", n);
		const char* szCommand = GetOption(optname);

		sprintf(optname, "Task%i.DownloadRate", n);
		const char* szDownloadRate = GetOption(optname);

		sprintf(optname, "Task%i.Process", n);
		const char* szProcess = GetOption(optname);

		sprintf(optname, "Task%i.Param", n);
		const char* szParam = GetOption(optname);

		if (Util::EmptyStr(szParam) && !Util::EmptyStr(szProcess))
		{
			szParam = szProcess;
		}
		if (Util::EmptyStr(szParam) && !Util::EmptyStr(szDownloadRate))
		{
			szParam = szDownloadRate;
		}

		bool definition = szTime || szWeekDays || szCommand || szDownloadRate || szParam;
		bool completed = szTime && szCommand;

		if (!definition)
		{
			break;
		}

		if (!completed)
		{
			ConfigError("Task definition not complete for \"Task%i\"", n);
			continue;
		}

		snprintf(optname, sizeof(optname), "Task%i.Command", n);
		optname[sizeof(optname)-1] = '\0';

		const char* CommandNames[] = { "pausedownload", "pause", "unpausedownload", "resumedownload", "unpause", "resume",
			"pausepostprocess", "unpausepostprocess", "resumepostprocess", "pausepost", "unpausepost", "resumepost",
			"downloadrate", "setdownloadrate", "rate", "speed", "script", "process", "pausescan", "unpausescan", "resumescan",
			"activateserver", "activateservers", "deactivateserver", "deactivateservers", "fetchfeed", "fetchfeeds" };
		const int CommandValues[] = { Scheduler::scPauseDownload, Scheduler::scPauseDownload, Scheduler::scUnpauseDownload,
			Scheduler::scUnpauseDownload, Scheduler::scUnpauseDownload, Scheduler::scUnpauseDownload,
			Scheduler::scPausePostProcess, Scheduler::scUnpausePostProcess, Scheduler::scUnpausePostProcess,
			Scheduler::scPausePostProcess, Scheduler::scUnpausePostProcess, Scheduler::scUnpausePostProcess,
			Scheduler::scDownloadRate, Scheduler::scDownloadRate, Scheduler::scDownloadRate, Scheduler::scDownloadRate,
			Scheduler::scScript, Scheduler::scProcess, Scheduler::scPauseScan, Scheduler::scUnpauseScan, Scheduler::scUnpauseScan,
			Scheduler::scActivateServer, Scheduler::scActivateServer, Scheduler::scDeactivateServer,
			Scheduler::scDeactivateServer, Scheduler::scFetchFeed, Scheduler::scFetchFeed };
		const int CommandCount = 27;
		Scheduler::ECommand eCommand = (Scheduler::ECommand)ParseEnumValue(optname, CommandCount, CommandNames, CommandValues);

		if (szParam && strlen(szParam) > 0 && eCommand == Scheduler::scProcess &&
			!Util::SplitCommandLine(szParam, NULL))
		{
			ConfigError("Invalid value for option \"Task%i.Param\"", n);
			continue;
		}

		int iWeekDays = 0;
		if (szWeekDays && !ParseWeekDays(szWeekDays, &iWeekDays))
		{
			ConfigError("Invalid value for option \"Task%i.WeekDays\": \"%s\"", n, szWeekDays);
			continue;
		}

		if (eCommand == Scheduler::scDownloadRate)
		{
			if (szParam)
			{
				char* szErr;
				int iDownloadRate = strtol(szParam, &szErr, 10);
				if (!szErr || *szErr != '\0' || iDownloadRate < 0)
				{
					ConfigError("Invalid value for option \"Task%i.Param\": \"%s\"", n, szDownloadRate);
					continue;
				}
			}
			else
			{
				ConfigError("Task definition not complete for \"Task%i\". Option \"Task%i.Param\" is missing", n, n);
				continue;
			}
		}

		if ((eCommand == Scheduler::scScript || 
			 eCommand == Scheduler::scProcess || 
			 eCommand == Scheduler::scActivateServer ||
			 eCommand == Scheduler::scDeactivateServer ||
			 eCommand == Scheduler::scFetchFeed) && 
			Util::EmptyStr(szParam))
		{
			ConfigError("Task definition not complete for \"Task%i\". Option \"Task%i.Param\" is missing", n, n);
			continue;
		}

		int iHours, iMinutes;
		Tokenizer tok(szTime, ";,");
		while (const char* szOneTime = tok.Next())
		{
			if (!ParseTime(szOneTime, &iHours, &iMinutes))
			{
				ConfigError("Invalid value for option \"Task%i.Time\": \"%s\"", n, szOneTime);
				break;
			}

			if (iHours == -1)
			{
				for (int iEveryHour = 0; iEveryHour < 24; iEveryHour++)
				{
					Scheduler::Task* pTask = new Scheduler::Task(n, iEveryHour, iMinutes, iWeekDays, eCommand, szParam);
					g_pScheduler->AddTask(pTask);
				}
			}
			else
			{
				Scheduler::Task* pTask = new Scheduler::Task(n, iHours, iMinutes, iWeekDays, eCommand, szParam);
				g_pScheduler->AddTask(pTask);
			}
		}
	}
}

bool Options::ParseTime(const char* szTime, int* pHours, int* pMinutes)
{
	int iColons = 0;
	const char* p = szTime;
	while (*p)
	{
		if (!strchr("0123456789: *", *p))
		{
			return false;
		}
		if (*p == ':')
		{
			iColons++;
		}
		p++;
	}

	if (iColons != 1)
	{
		return false;
	}

	const char* szColon = strchr(szTime, ':');
	if (!szColon)
	{
		return false;
	}

	if (szTime[0] == '*')
	{
		*pHours = -1;
	}
	else
	{
		*pHours = atoi(szTime);
		if (*pHours < 0 || *pHours > 23)
		{
			return false;
		}
	}

	if (szColon[1] == '*')
	{
		return false;
	}
	*pMinutes = atoi(szColon + 1);
	if (*pMinutes < 0 || *pMinutes > 59)
	{
		return false;
	}

	return true;
}

bool Options::ParseWeekDays(const char* szWeekDays, int* pWeekDaysBits)
{
	*pWeekDaysBits = 0;
	const char* p = szWeekDays;
	int iFirstDay = 0;
	bool bRange = false;
	while (*p)
	{
		if (strchr("1234567", *p))
		{
			int iDay = *p - '0';
			if (bRange)
			{
				if (iDay <= iFirstDay || iFirstDay == 0)
				{
					return false;
				}
				for (int i = iFirstDay; i <= iDay; i++)
				{
					*pWeekDaysBits |= 1 << (i - 1);
				}
				iFirstDay = 0;
			}
			else
			{
				*pWeekDaysBits |= 1 << (iDay - 1);
				iFirstDay = iDay;
			}
			bRange = false;
		}
		else if (*p == ',')
		{
			bRange = false;
		}
		else if (*p == '-')
		{
			bRange = true;
		}
		else if (*p == ' ')
		{
			// skip spaces
		}
		else
		{
			return false;
		}
		p++;
	}
	return true;
}

void Options::LoadConfigFile()
{
	FILE* infile = fopen(m_szConfigFilename, FOPEN_RB);

	if (!infile)
	{
		abort("FATAL ERROR: Could not open file %s\n", m_szConfigFilename);
	}

	m_iConfigLine = 0;
	int iBufLen = (int)Util::FileSize(m_szConfigFilename) + 1;
	char* buf = (char*)malloc(iBufLen);

	int iLine = 0;
	while (fgets(buf, iBufLen - 1, infile))
	{
		m_iConfigLine = ++iLine;

		if (buf[0] != 0 && buf[strlen(buf)-1] == '\n')
		{
			buf[strlen(buf)-1] = 0; // remove traling '\n'
		}
		if (buf[0] != 0 && buf[strlen(buf)-1] == '\r')
		{
			buf[strlen(buf)-1] = 0; // remove traling '\r' (for windows line endings)
		}

		if (buf[0] == 0 || buf[0] == '#' || strspn(buf, " ") == strlen(buf))
		{
			continue;
		}

		SetOptionString(buf);
	}

	fclose(infile);
	free(buf);

	m_iConfigLine = 0;
}

bool Options::SetOptionString(const char* option)
{
	char* optname;
	char* optvalue;

	if (!SplitOptionString(option, &optname, &optvalue))
	{
		ConfigError("Invalid option \"%s\"", option);
		return false;
	}

	bool bOK = ValidateOptionName(optname, optvalue);
	if (bOK)
	{
		SetOption(optname, optvalue);
	}
	else
	{
		ConfigError("Invalid option \"%s\"", optname);
	}

	free(optname);
	free(optvalue);

	return bOK;
}

/*
 * Splits option string into name and value;
 * Converts old names and values if necessary;
 * Allocates buffers for name and value;
 * Returns true if the option string has name and value;
 * If "true" is returned the caller is responsible for freeing optname and optvalue.
 */
bool Options::SplitOptionString(const char* option, char** pOptName, char** pOptValue)
{
	const char* eq = strchr(option, '=');
	if (!eq)
	{
		return false;
	}

	const char* value = eq + 1;

	char optname[1001];
	char optvalue[1001];
	int maxlen = (int)(eq - option < 1000 ? eq - option : 1000);
	strncpy(optname, option, maxlen);
	optname[maxlen] = '\0';
	strncpy(optvalue, eq + 1, 1000);
	optvalue[1000]  = '\0';
	if (strlen(optname) == 0)
	{
		return false;
	}

	ConvertOldOption(optname, sizeof(optname), optvalue, sizeof(optvalue));

	// if value was (old-)converted use the new value, which is linited to 1000 characters,
	// otherwise use original (length-unlimited) value
	if (strncmp(value, optvalue, 1000))
	{
		value = optvalue;
	}

	*pOptName = strdup(optname);
	*pOptValue = strdup(value);

	return true;
}

bool Options::ValidateOptionName(const char* optname, const char* optvalue)
{
	if (!strcasecmp(optname, OPTION_CONFIGFILE) || !strcasecmp(optname, OPTION_APPBIN) ||
		!strcasecmp(optname, OPTION_APPDIR) || !strcasecmp(optname, OPTION_VERSION))
	{
		// read-only options
		return false;
	}

	const char* v = GetOption(optname);
	if (v)
	{
		// it's predefined option, OK
		return true;
	}

	if (!strncasecmp(optname, "server", 6))
	{
		char* p = (char*)optname + 6;
		while (*p >= '0' && *p <= '9') p++;
		if (p &&
			(!strcasecmp(p, ".active") || !strcasecmp(p, ".name") ||
			!strcasecmp(p, ".level") || !strcasecmp(p, ".host") ||
			!strcasecmp(p, ".port") || !strcasecmp(p, ".username") ||
			!strcasecmp(p, ".password") || !strcasecmp(p, ".joingroup") ||
			!strcasecmp(p, ".encryption") || !strcasecmp(p, ".connections") ||
			!strcasecmp(p, ".cipher") || !strcasecmp(p, ".group") ||
			!strcasecmp(p, ".retention")))
		{
			return true;
		}
	}

	if (!strncasecmp(optname, "task", 4))
	{
		char* p = (char*)optname + 4;
		while (*p >= '0' && *p <= '9') p++;
		if (p && (!strcasecmp(p, ".time") || !strcasecmp(p, ".weekdays") ||
			!strcasecmp(p, ".command") || !strcasecmp(p, ".param") ||
			!strcasecmp(p, ".downloadrate") || !strcasecmp(p, ".process")))
		{
			return true;
		}
	}
	
	if (!strncasecmp(optname, "category", 8))
	{
		char* p = (char*)optname + 8;
		while (*p >= '0' && *p <= '9') p++;
		if (p && (!strcasecmp(p, ".name") || !strcasecmp(p, ".destdir") || !strcasecmp(p, ".postscript") ||
			!strcasecmp(p, ".unpack") || !strcasecmp(p, ".aliases")))
		{
			return true;
		}
	}

	if (!strncasecmp(optname, "feed", 4))
	{
		char* p = (char*)optname + 4;
		while (*p >= '0' && *p <= '9') p++;
		if (p && (!strcasecmp(p, ".name") || !strcasecmp(p, ".url") || !strcasecmp(p, ".interval") ||
			 !strcasecmp(p, ".filter") || !strcasecmp(p, ".pausenzb") || !strcasecmp(p, ".category") ||
			 !strcasecmp(p, ".priority")))
		{
			return true;
		}
	}

	// scripts options
	if (strchr(optname, ':'))
	{
		return true;
	}

	// print warning messages for obsolete options
	if (!strcasecmp(optname, OPTION_RETRYONCRCERROR) ||
		!strcasecmp(optname, OPTION_ALLOWREPROCESS) ||
		!strcasecmp(optname, OPTION_LOADPARS) ||
		!strcasecmp(optname, OPTION_THREADLIMIT) ||
		!strcasecmp(optname, OPTION_POSTLOGKIND) ||
		!strcasecmp(optname, OPTION_NZBLOGKIND) ||
		!strcasecmp(optname, OPTION_PROCESSLOGKIND) ||
		!strcasecmp(optname, OPTION_APPENDNZBDIR) ||
		!strcasecmp(optname, OPTION_RENAMEBROKEN) ||
		!strcasecmp(optname, OPTION_MERGENZB) ||
		!strcasecmp(optname, OPTION_STRICTPARNAME) ||
		!strcasecmp(optname, OPTION_RELOADURLQUEUE) ||
		!strcasecmp(optname, OPTION_RELOADPOSTQUEUE))
	{
		ConfigWarn("Option \"%s\" is obsolete, ignored", optname);
		return true;
	}
	if (!strcasecmp(optname, OPTION_POSTPROCESS) ||
		!strcasecmp(optname, OPTION_NZBPROCESS) ||
		!strcasecmp(optname, OPTION_NZBADDEDPROCESS))
	{
		if (optvalue && strlen(optvalue) > 0)
		{
			ConfigError("Option \"%s\" is obsolete, ignored, use \"%s\" and \"%s\" instead", optname, OPTION_SCRIPTDIR,
				!strcasecmp(optname, OPTION_POSTPROCESS) ? OPTION_POSTSCRIPT :
				!strcasecmp(optname, OPTION_NZBPROCESS) ? OPTION_SCANSCRIPT :
				!strcasecmp(optname, OPTION_NZBADDEDPROCESS) ? OPTION_QUEUESCRIPT :
				"ERROR");
		}
		return true;
	}

	if (!strcasecmp(optname, OPTION_CREATELOG) || !strcasecmp(optname, OPTION_RESETLOG))
	{
		ConfigWarn("Option \"%s\" is obsolete, ignored, use \"%s\" instead", optname, OPTION_WRITELOG);
		return true;
	}

	return false;
}

void Options::ConvertOldOption(char *szOption, int iOptionBufLen, char *szValue, int iValueBufLen)
{
	// for compatibility with older versions accept old option names

	if (!strcasecmp(szOption, "$MAINDIR"))
	{
		strncpy(szOption, "MainDir", iOptionBufLen);
	}

	if (!strcasecmp(szOption, "ServerIP"))
	{
		strncpy(szOption, "ControlIP", iOptionBufLen);
	}

	if (!strcasecmp(szOption, "ServerPort"))
	{
		strncpy(szOption, "ControlPort", iOptionBufLen);
	}

	if (!strcasecmp(szOption, "ServerPassword"))
	{
		strncpy(szOption, "ControlPassword", iOptionBufLen);
	}

	if (!strcasecmp(szOption, "PostPauseQueue"))
	{
		strncpy(szOption, "ScriptPauseQueue", iOptionBufLen);
	}

	if (!strcasecmp(szOption, "ParCheck") && !strcasecmp(szValue, "yes"))
	{
		strncpy(szValue, "always", iValueBufLen);
	}

	if (!strcasecmp(szOption, "ParCheck") && !strcasecmp(szValue, "no"))
	{
		strncpy(szValue, "auto", iValueBufLen);
	}

	if (!strcasecmp(szOption, "DefScript"))
	{
		strncpy(szOption, "PostScript", iOptionBufLen);
	}

	int iNameLen = strlen(szOption);
	if (!strncasecmp(szOption, "Category", 8) && iNameLen > 10 &&
		!strcasecmp(szOption + iNameLen - 10, ".DefScript"))
	{
		strncpy(szOption + iNameLen - 10, ".PostScript", iOptionBufLen - 9 /* strlen("Category.") */);
	}

	if (!strcasecmp(szOption, "WriteBufferSize"))
	{
		strncpy(szOption, "WriteBuffer", iOptionBufLen);
		int val = strtol(szValue, NULL, 10);
		val = val == -1 ? 1024 : val / 1024;
		snprintf(szValue, iValueBufLen, "%i", val);
	}	

	if (!strcasecmp(szOption, "ConnectionTimeout"))
	{
		strncpy(szOption, "ArticleTimeout", iOptionBufLen);
	}

	if (!strcasecmp(szOption, "CreateBrokenLog"))
	{
		strncpy(szOption, "BrokenLog", iOptionBufLen);
	}

	szOption[iOptionBufLen-1] = '\0';
	szOption[iValueBufLen-1] = '\0';
}

void Options::CheckOptions()
{
#ifdef DISABLE_PARCHECK
	if (m_eParCheck != pcManual)
	{
		LocateOptionSrcPos(OPTION_PARCHECK);
		ConfigError("Invalid value for option \"%s\": program was compiled without parcheck-support", OPTION_PARCHECK);
	}
	if (m_bParRename)
	{
		LocateOptionSrcPos(OPTION_PARRENAME);
		ConfigError("Invalid value for option \"%s\": program was compiled without parcheck-support", OPTION_PARRENAME);
	}
#endif

#ifdef DISABLE_CURSES
	if (m_eOutputMode == omNCurses)
	{
		LocateOptionSrcPos(OPTION_OUTPUTMODE);
		ConfigError("Invalid value for option \"%s\": program was compiled without curses-support", OPTION_OUTPUTMODE);
	}
#endif

#ifdef DISABLE_TLS
	if (m_bSecureControl)
	{
		LocateOptionSrcPos(OPTION_SECURECONTROL);
		ConfigError("Invalid value for option \"%s\": program was compiled without TLS/SSL-support", OPTION_SECURECONTROL);
	}
#endif

	if (!m_bDecode)
	{
		m_bDirectWrite = false;
	}

	// if option "ConfigTemplate" is not set, use "WebDir" as default location for template
	// (for compatibility with versions 9 and 10).
	if (!m_szConfigTemplate || m_szConfigTemplate[0] == '\0')
	{
		free(m_szConfigTemplate);
		int iLen = strlen(m_szWebDir) + 15;
		m_szConfigTemplate = (char*)malloc(iLen);
		snprintf(m_szConfigTemplate, iLen, "%s%s", m_szWebDir, "nzbget.conf");
		m_szConfigTemplate[iLen-1] = '\0';
		if (!Util::FileExists(m_szConfigTemplate))
		{
			free(m_szConfigTemplate);
			m_szConfigTemplate = strdup("");
		}
	}

	if (m_iArticleCache < 0)
	{
		m_iArticleCache = 0;
	}
	else if (sizeof(void*) == 4 && m_iArticleCache > 1900)
	{
		ConfigError("Invalid value for option \"ArticleCache\": %i. Changed to 1900", m_iArticleCache);
		m_iArticleCache = 1900;
	}
	else if (sizeof(void*) == 4 && m_iParBuffer > 1900)
	{
		ConfigError("Invalid value for option \"ParBuffer\": %i. Changed to 1900", m_iParBuffer);
		m_iParBuffer = 1900;
	}

	if (sizeof(void*) == 4 && m_iParBuffer + m_iArticleCache > 1900)
	{
		ConfigError("Options \"ArticleCache\" and \"ParBuffer\" in total cannot use more than 1900MB of memory in 32-Bit mode. Changed to 1500 and 400");
		m_iArticleCache = 1900;
		m_iParBuffer = 400;
	}

	if (!Util::EmptyStr(m_szUnpackPassFile) && !Util::FileExists(m_szUnpackPassFile))
	{
		ConfigError("Invalid value for option \"UnpackPassFile\": %s. File not found", m_szUnpackPassFile);
	}
}

void Options::ParseFileIDList(int argc, char* argv[], int optind)
{
	std::vector<int> IDs;
	IDs.clear();

	while (optind < argc)
	{
		char* szWritableFileIDList = strdup(argv[optind++]);

		char* optarg = strtok(szWritableFileIDList, ", ");
		while (optarg)
		{
			int iEditQueueIDFrom = 0;
			int iEditQueueIDTo = 0;
			const char* p = strchr(optarg, '-');
			if (p)
			{
				char buf[101];
				int maxlen = (int)(p - optarg < 100 ? p - optarg : 100);
				strncpy(buf, optarg, maxlen);
				buf[maxlen] = '\0';
				iEditQueueIDFrom = atoi(buf);
				iEditQueueIDTo = atoi(p + 1);
				if (iEditQueueIDFrom <= 0 || iEditQueueIDTo <= 0)
				{
					abort("FATAL ERROR: invalid list of file IDs\n");
				}
			}
			else
			{
				iEditQueueIDFrom = atoi(optarg);
				if (iEditQueueIDFrom <= 0)
				{
					abort("FATAL ERROR: invalid list of file IDs\n");
				}
				iEditQueueIDTo = iEditQueueIDFrom;
			}

			int iEditQueueIDCount = 0;
			if (iEditQueueIDTo != 0)
			{
				if (iEditQueueIDFrom < iEditQueueIDTo)
				{
					iEditQueueIDCount = iEditQueueIDTo - iEditQueueIDFrom + 1;
				}
				else
				{
					iEditQueueIDCount = iEditQueueIDFrom - iEditQueueIDTo + 1;
				}
			}
			else
			{
				iEditQueueIDCount = 1;
			}

			for (int i = 0; i < iEditQueueIDCount; i++)
			{
				if (iEditQueueIDFrom < iEditQueueIDTo || iEditQueueIDTo == 0)
				{
					IDs.push_back(iEditQueueIDFrom + i);
				}
				else
				{
					IDs.push_back(iEditQueueIDFrom - i);
				}
			}

			optarg = strtok(NULL, ", ");
		}

		free(szWritableFileIDList);
	}

	m_iEditQueueIDCount = IDs.size();
	m_pEditQueueIDList = (int*)malloc(sizeof(int) * m_iEditQueueIDCount);
	for (int i = 0; i < m_iEditQueueIDCount; i++)
	{
		m_pEditQueueIDList[i] = IDs[i];
	}
}

void Options::ParseFileNameList(int argc, char* argv[], int optind)
{
	while (optind < argc)
	{
		m_EditQueueNameList.push_back(strdup(argv[optind++]));
	}
}

Options::OptEntries* Options::LockOptEntries()
{
	m_mutexOptEntries.Lock();
	return &m_OptEntries;
}

void Options::UnlockOptEntries()
{
	m_mutexOptEntries.Unlock();
}

bool Options::LoadConfig(OptEntries* pOptEntries)
{
	// read config file
	FILE* infile = fopen(m_szConfigFilename, FOPEN_RB);

	if (!infile)
	{
		return false;
	}

	int iBufLen = (int)Util::FileSize(m_szConfigFilename) + 1;
	char* buf = (char*)malloc(iBufLen);

	while (fgets(buf, iBufLen - 1, infile))
	{
		// remove trailing '\n' and '\r' and spaces
		Util::TrimRight(buf);

		// skip comments and empty lines
		if (buf[0] == 0 || buf[0] == '#' || strspn(buf, " ") == strlen(buf))
		{
			continue;
		}

		char* optname;
		char* optvalue;
		if (SplitOptionString(buf, &optname, &optvalue))
		{
			OptEntry* pOptEntry = new OptEntry();
			pOptEntry->SetName(optname);
			pOptEntry->SetValue(optvalue);
			pOptEntries->push_back(pOptEntry);

			free(optname);
			free(optvalue);
		}
	}

	fclose(infile);
	free(buf);

	return true;
}

bool Options::SaveConfig(OptEntries* pOptEntries)
{
	// save to config file
	FILE* infile = fopen(m_szConfigFilename, FOPEN_RBP);

	if (!infile)
	{
		return false;
	}

	std::vector<char*> config;
	std::set<OptEntry*> writtenOptions;

	// read config file into memory array
	int iBufLen = (int)Util::FileSize(m_szConfigFilename) + 1;
	char* buf = (char*)malloc(iBufLen);
	while (fgets(buf, iBufLen - 1, infile))
	{
		config.push_back(strdup(buf));
	}
	free(buf);

	// write config file back to disk, replace old values of existing options with new values
	rewind(infile);
	for (std::vector<char*>::iterator it = config.begin(); it != config.end(); it++)
    {
        char* buf = *it;

		const char* eq = strchr(buf, '=');
		if (eq && buf[0] != '#')
		{
			// remove trailing '\n' and '\r' and spaces
			Util::TrimRight(buf);

			char* optname;
			char* optvalue;
			if (SplitOptionString(buf, &optname, &optvalue))
			{
				OptEntry *pOptEntry = pOptEntries->FindOption(optname);
				if (pOptEntry)
				{
					fputs(pOptEntry->GetName(), infile);
					fputs("=", infile);
					fputs(pOptEntry->GetValue(), infile);
					fputs("\n", infile);
					writtenOptions.insert(pOptEntry);
				}

				free(optname);
				free(optvalue);
			}
		}
		else
		{
			fputs(buf, infile);
		}

		free(buf);
	}

	// write new options
	for (Options::OptEntries::iterator it = pOptEntries->begin(); it != pOptEntries->end(); it++)
	{
		Options::OptEntry* pOptEntry = *it;
		std::set<OptEntry*>::iterator fit = writtenOptions.find(pOptEntry);
		if (fit == writtenOptions.end())
		{
			fputs(pOptEntry->GetName(), infile);
			fputs("=", infile);
			fputs(pOptEntry->GetValue(), infile);
			fputs("\n", infile);
		}
	}

	// close and truncate the file
	int pos = (int)ftell(infile);
	fclose(infile);

	Util::TruncateFile(m_szConfigFilename, pos);

	return true;
}

bool Options::LoadConfigTemplates(ConfigTemplates* pConfigTemplates)
{
	char* szBuffer;
	int iLength;
	if (!Util::LoadFileIntoBuffer(m_szConfigTemplate, &szBuffer, &iLength))
	{
		return false;
	}
	ConfigTemplate* pConfigTemplate = new ConfigTemplate(NULL, szBuffer);
	pConfigTemplates->push_back(pConfigTemplate);
	free(szBuffer);

	if (!m_szScriptDir)
	{
		return true;
	}

	Scripts scriptList;
	LoadScripts(&scriptList);

	const int iBeginSignatureLen = strlen(BEGIN_SCRIPT_SIGNATURE);
	const int iQueueEventsSignatureLen = strlen(QUEUE_EVENTS_SIGNATURE);

	for (Scripts::iterator it = scriptList.begin(); it != scriptList.end(); it++)
	{
		Script* pScript = *it;

		FILE* infile = fopen(pScript->GetLocation(), FOPEN_RB);
		if (!infile)
		{
			ConfigTemplate* pConfigTemplate = new ConfigTemplate(pScript, "");
			pConfigTemplates->push_back(pConfigTemplate);
			continue;
		}

		StringBuilder stringBuilder;
		char buf[1024];
		bool bInConfig = false;

		while (fgets(buf, sizeof(buf) - 1, infile))
		{
			if (!strncmp(buf, BEGIN_SCRIPT_SIGNATURE, iBeginSignatureLen) &&
				strstr(buf, END_SCRIPT_SIGNATURE) &&
				(strstr(buf, POST_SCRIPT_SIGNATURE) ||
				 strstr(buf, SCAN_SCRIPT_SIGNATURE) ||
				 strstr(buf, QUEUE_SCRIPT_SIGNATURE) ||
				 strstr(buf, SCHEDULER_SCRIPT_SIGNATURE)))
			{
				if (bInConfig)
				{
					break;
				}
				bInConfig = true;
				continue;
			}

			bool bSkip = !strncmp(buf, QUEUE_EVENTS_SIGNATURE, iQueueEventsSignatureLen);

			if (bInConfig && !bSkip)
			{
				stringBuilder.Append(buf);
			}
		}

		fclose(infile);

		ConfigTemplate* pConfigTemplate = new ConfigTemplate(pScript, stringBuilder.GetBuffer());
		pConfigTemplates->push_back(pConfigTemplate);
	}

	// clearing the list without deleting of objects, which are in pConfigTemplates now 
	scriptList.clear();

	return true;
}

void Options::InitConfigTemplates()
{
	if (!LoadConfigTemplates(&m_ConfigTemplates))
	{
		error("Could not read configuration templates");
	}
}

void Options::InitScripts()
{
	LoadScripts(&m_Scripts);
}

void Options::LoadScripts(Scripts* pScripts)
{
	if (strlen(m_szScriptDir) == 0)
	{
		return;
	}

	Scripts tmpScripts;
	LoadScriptDir(&tmpScripts, m_szScriptDir, false);
	tmpScripts.sort(CompareScripts);

	// first add all scripts from m_szScriptOrder
	Tokenizer tok(m_szScriptOrder, ",;");
	while (const char* szScriptName = tok.Next())
	{
		Script* pScript = tmpScripts.Find(szScriptName);
		if (pScript)
		{
			tmpScripts.remove(pScript);
			pScripts->push_back(pScript);
		}
	}

	// second add all other scripts from scripts directory
	for (Scripts::iterator it = tmpScripts.begin(); it != tmpScripts.end(); it++)
	{
		Script* pScript = *it;
		if (!pScripts->Find(pScript->GetName()))
		{
			pScripts->push_back(pScript);
		}
	}

	tmpScripts.clear();

	BuildScriptDisplayNames(pScripts);
}

void Options::LoadScriptDir(Scripts* pScripts, const char* szDirectory, bool bIsSubDir)
{
	int iBufSize = 1024*10;
	char* szBuffer = (char*)malloc(iBufSize+1);

	const int iBeginSignatureLen = strlen(BEGIN_SCRIPT_SIGNATURE);
	const int iQueueEventsSignatureLen = strlen(QUEUE_EVENTS_SIGNATURE);

	DirBrowser dir(szDirectory);
	while (const char* szFilename = dir.Next())
	{
		if (szFilename[0] != '.' && szFilename[0] != '_')
		{
			char szFullFilename[1024];
			snprintf(szFullFilename, 1024, "%s%s", szDirectory, szFilename);
			szFullFilename[1024-1] = '\0';

			if (!Util::DirectoryExists(szFullFilename))
			{
				// check if the file contains pp-script-signature
				FILE* infile = fopen(szFullFilename, FOPEN_RB);
				if (infile)
				{
					// read first 10KB of the file and look for signature
					int iReadBytes = fread(szBuffer, 1, iBufSize, infile);
					fclose(infile);
					szBuffer[iReadBytes] = 0;

					// split buffer into lines
					Tokenizer tok(szBuffer, "\n\r", true);
					while (char* szLine = tok.Next())
					{
						if (!strncmp(szLine, BEGIN_SCRIPT_SIGNATURE, iBeginSignatureLen) &&
							strstr(szLine, END_SCRIPT_SIGNATURE))
						{
							bool bPostScript = strstr(szLine, POST_SCRIPT_SIGNATURE);
							bool bScanScript = strstr(szLine, SCAN_SCRIPT_SIGNATURE);
							bool bQueueScript = strstr(szLine, QUEUE_SCRIPT_SIGNATURE);
							bool bSchedulerScript = strstr(szLine, SCHEDULER_SCRIPT_SIGNATURE);
							if (bPostScript || bScanScript || bQueueScript || bSchedulerScript)
							{
								char szScriptName[1024];
								if (bIsSubDir)
								{
									char szDirectory2[1024];
									snprintf(szDirectory2, 1024, "%s", szDirectory);
									szDirectory2[1024-1] = '\0';
									int iLen = strlen(szDirectory2);
									if (szDirectory2[iLen-1] == PATH_SEPARATOR || szDirectory2[iLen-1] == ALT_PATH_SEPARATOR)
									{
										// trim last path-separator
										szDirectory2[iLen-1] = '\0';
									}

									snprintf(szScriptName, 1024, "%s%c%s", Util::BaseFileName(szDirectory2), PATH_SEPARATOR, szFilename);
								}
								else
								{
									snprintf(szScriptName, 1024, "%s", szFilename);
								}
								szScriptName[1024-1] = '\0';

								char* szQueueEvents = NULL;
								if (bQueueScript)
								{
									while (char* szLine = tok.Next())
									{
										if (!strncmp(szLine, QUEUE_EVENTS_SIGNATURE, iQueueEventsSignatureLen))
										{
											szQueueEvents = szLine + iQueueEventsSignatureLen;
											break;
										}
									}
								}

								Script* pScript = new Script(szScriptName, szFullFilename);
								pScript->SetPostScript(bPostScript);
								pScript->SetScanScript(bScanScript);
								pScript->SetQueueScript(bQueueScript);
								pScript->SetSchedulerScript(bSchedulerScript);
								pScript->SetQueueEvents(szQueueEvents);
								pScripts->push_back(pScript);
								break;
							}
						}
					}
				}
			}
			else if (!bIsSubDir)
			{
				snprintf(szFullFilename, 1024, "%s%s%c", szDirectory, szFilename, PATH_SEPARATOR);
				szFullFilename[1024-1] = '\0';

				LoadScriptDir(pScripts, szFullFilename, true);
			}
		}
	}

	free(szBuffer);
}

bool Options::CompareScripts(Script* pScript1, Script* pScript2)
{
	return strcmp(pScript1->GetName(), pScript2->GetName()) < 0;
}

void Options::BuildScriptDisplayNames(Scripts* pScripts)
{
	// trying to use short name without path and extension.
	// if there are other scripts with the same short name - using a longer name instead (with ot without extension)

	for (Scripts::iterator it = pScripts->begin(); it != pScripts->end(); it++)
	{
		Script* pScript = *it;

		char szShortName[256];
		strncpy(szShortName, pScript->GetName(), 256);
		szShortName[256-1] = '\0';
		if (char* ext = strrchr(szShortName, '.')) *ext = '\0'; // strip file extension

		const char* szDisplayName = Util::BaseFileName(szShortName);

		for (Scripts::iterator it2 = pScripts->begin(); it2 != pScripts->end(); it2++)
		{
			Script* pScript2 = *it2;

			char szShortName2[256];
			strncpy(szShortName2, pScript2->GetName(), 256);
			szShortName2[256-1] = '\0';
			if (char* ext = strrchr(szShortName2, '.')) *ext = '\0'; // strip file extension

			const char* szDisplayName2 = Util::BaseFileName(szShortName2);

			if (!strcmp(szDisplayName, szDisplayName2) && pScript->GetName() != pScript2->GetName())
			{
				if (!strcmp(szShortName, szShortName2))
				{
					szDisplayName =	pScript->GetName();
				}
				else
				{
					szDisplayName =	szShortName;
				}
				break;
			}
		}

		pScript->SetDisplayName(szDisplayName);
	}
}
