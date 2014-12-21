/*
 *  This file is part of nzbget
 *
 *  Copyright (C) 2008-2014 Andrey Prygunkov <hugbug@users.sourceforge.net>
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
 * $Revision: 1124 $
 * $Date: 2014-09-15 18:28:55 +0200 (Mon, 15 Sep 2014) $
 *
 */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef WIN32
#include "win32.h"
#else
#include <unistd.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "nzbget.h"
#include "Scheduler.h"
#include "Options.h"
#include "Log.h"
#include "NewsServer.h"
#include "ServerPool.h"
#include "FeedInfo.h"
#include "FeedCoordinator.h"
#include "QueueScript.h"

extern Options* g_pOptions;
extern ServerPool* g_pServerPool;
extern FeedCoordinator* g_pFeedCoordinator;

class SchedulerScriptController : public Thread, public NZBScriptController
{
private:
	char*				m_szScript;
	bool				m_bExternalProcess;
	int					m_iTaskID;

	void				PrepareParams(const char* szScriptName);
	void				ExecuteExternalProcess();

protected:
	virtual void		ExecuteScript(Options::Script* pScript);

public:
	virtual				~SchedulerScriptController();
	virtual void		Run();
	static void			StartScript(const char* szParam, bool bExternalProcess, int iTaskID);
};

Scheduler::Task::Task(int iID, int iHours, int iMinutes, int iWeekDaysBits, ECommand eCommand, const char* szParam)
{
	m_iID = iID;
	m_iHours = iHours;
	m_iMinutes = iMinutes;
	m_iWeekDaysBits = iWeekDaysBits;
	m_eCommand = eCommand;
	m_szParam = szParam ? strdup(szParam) : NULL;
	m_tLastExecuted = 0;
}

Scheduler::Task::~Task()
{
	free(m_szParam);
}


Scheduler::Scheduler()
{
	debug("Creating Scheduler");

	m_tLastCheck = 0;
	m_TaskList.clear();
}

Scheduler::~Scheduler()
{
	debug("Destroying Scheduler");
	
	for (TaskList::iterator it = m_TaskList.begin(); it != m_TaskList.end(); it++)
	{
		delete *it;
	}
}

void Scheduler::AddTask(Task* pTask)
{
	m_mutexTaskList.Lock();
	m_TaskList.push_back(pTask);
	m_mutexTaskList.Unlock();
}

bool Scheduler::CompareTasks(Scheduler::Task* pTask1, Scheduler::Task* pTask2)
{
	return (pTask1->m_iHours < pTask2->m_iHours) || 
		((pTask1->m_iHours == pTask2->m_iHours) && (pTask1->m_iMinutes < pTask2->m_iMinutes));
}

void Scheduler::FirstCheck()
{
	m_mutexTaskList.Lock();
	m_TaskList.sort(CompareTasks);
	m_mutexTaskList.Unlock();

	// check all tasks for the last week
	CheckTasks();
}

void Scheduler::IntervalCheck()
{
	m_bExecuteProcess = true;
	CheckTasks();
	CheckScheduledResume();
}

void Scheduler::CheckTasks()
{
	PrepareLog();

	m_mutexTaskList.Lock();

	time_t tCurrent = time(NULL);

	if (!m_TaskList.empty())
	{
		// Detect large step changes of system time 
		time_t tDiff = tCurrent - m_tLastCheck;
		if (tDiff > 60*90 || tDiff < 0)
		{
			debug("Reset scheduled tasks (detected clock change greater than 90 minutes or negative)");

			// check all tasks for the last week
			m_tLastCheck = tCurrent - 60*60*24*7;
			m_bExecuteProcess = false;

			for (TaskList::iterator it = m_TaskList.begin(); it != m_TaskList.end(); it++)
			{
				Task* pTask = *it;
				pTask->m_tLastExecuted = 0;
			}
		}

		time_t tLocalCurrent = tCurrent + g_pOptions->GetLocalTimeOffset();
		time_t tLocalLastCheck = m_tLastCheck + g_pOptions->GetLocalTimeOffset();

		tm tmCurrent;
		gmtime_r(&tLocalCurrent, &tmCurrent);
		tm tmLastCheck;
		gmtime_r(&tLocalLastCheck, &tmLastCheck);

		tm tmLoop;
		memcpy(&tmLoop, &tmLastCheck, sizeof(tmLastCheck));
		tmLoop.tm_hour = tmCurrent.tm_hour;
		tmLoop.tm_min = tmCurrent.tm_min;
		tmLoop.tm_sec = tmCurrent.tm_sec;
		time_t tLoop = Util::Timegm(&tmLoop);

		while (tLoop <= tLocalCurrent)
		{
			for (TaskList::iterator it = m_TaskList.begin(); it != m_TaskList.end(); it++)
			{
				Task* pTask = *it;
				if (pTask->m_tLastExecuted != tLoop)
				{
					tm tmAppoint;
					memcpy(&tmAppoint, &tmLoop, sizeof(tmLoop));
					tmAppoint.tm_hour = pTask->m_iHours;
					tmAppoint.tm_min = pTask->m_iMinutes;
					tmAppoint.tm_sec = 0;

					time_t tAppoint = Util::Timegm(&tmAppoint);

					int iWeekDay = tmAppoint.tm_wday;
					if (iWeekDay == 0)
					{
						iWeekDay = 7;
					}

					bool bWeekDayOK = pTask->m_iWeekDaysBits == 0 || (pTask->m_iWeekDaysBits & (1 << (iWeekDay - 1)));
					bool bDoTask = bWeekDayOK && tLocalLastCheck < tAppoint && tAppoint <= tLocalCurrent;

					//debug("TEMP: 1) m_tLastCheck=%i, tLocalCurrent=%i, tLoop=%i, tAppoint=%i, bWeekDayOK=%i, bDoTask=%i", m_tLastCheck, tLocalCurrent, tLoop, tAppoint, (int)bWeekDayOK, (int)bDoTask);

					if (bDoTask)
					{
						ExecuteTask(pTask);
						pTask->m_tLastExecuted = tLoop;
					}
				}
			}
			tLoop += 60*60*24; // inc day
			gmtime_r(&tLoop, &tmLoop);
		}
	}

	m_tLastCheck = tCurrent;

	m_mutexTaskList.Unlock();

	PrintLog();
}

void Scheduler::ExecuteTask(Task* pTask)
{
	const char* szCommandName[] = { "Pause", "Unpause", "Pause Post-processing", "Unpause Post-processing",
		"Set download rate", "Execute process", "Execute script",
		"Pause Scan", "Unpause Scan", "Enable Server", "Disable Server", "Fetch Feed" };
	debug("Executing scheduled command: %s", szCommandName[pTask->m_eCommand]);

	switch (pTask->m_eCommand)
	{
		case scDownloadRate:
			if (!Util::EmptyStr(pTask->m_szParam))
			{
				g_pOptions->SetDownloadRate(atoi(pTask->m_szParam) * 1024);
				m_bDownloadRateChanged = true;
			}
			break;

		case scPauseDownload:
		case scUnpauseDownload:
			g_pOptions->SetPauseDownload(pTask->m_eCommand == scPauseDownload);
			m_bPauseDownloadChanged = true;
			break;

		case scPausePostProcess:
		case scUnpausePostProcess:
			g_pOptions->SetPausePostProcess(pTask->m_eCommand == scPausePostProcess);
			m_bPausePostProcessChanged = true;
			break;

		case scPauseScan:
		case scUnpauseScan:
			g_pOptions->SetPauseScan(pTask->m_eCommand == scPauseScan);
			m_bPauseScanChanged = true;
			break;

		case scScript:
		case scProcess:
			if (m_bExecuteProcess)
			{
				SchedulerScriptController::StartScript(pTask->m_szParam, pTask->m_eCommand == scProcess, pTask->m_iID);
			}
			break;

		case scActivateServer:
		case scDeactivateServer:
			EditServer(pTask->m_eCommand == scActivateServer, pTask->m_szParam);
			break;

		case scFetchFeed:
			if (m_bExecuteProcess)
			{
				FetchFeed(pTask->m_szParam);
				break;
			}
	}
}

void Scheduler::PrepareLog()
{
	m_bDownloadRateChanged = false;
	m_bPauseDownloadChanged = false;
	m_bPausePostProcessChanged = false;
	m_bPauseScanChanged = false;
	m_bServerChanged = false;
}

void Scheduler::PrintLog()
{
	if (m_bDownloadRateChanged)
	{
		info("Scheduler: setting download rate to %i KB/s", g_pOptions->GetDownloadRate() / 1024);
	}
	if (m_bPauseDownloadChanged)
	{
		info("Scheduler: %s download", g_pOptions->GetPauseDownload() ? "pausing" : "unpausing");
	}
	if (m_bPausePostProcessChanged)
	{
		info("Scheduler: %s post-processing", g_pOptions->GetPausePostProcess() ? "pausing" : "unpausing");
	}
	if (m_bPauseScanChanged)
	{
		info("Scheduler: %s scan", g_pOptions->GetPauseScan() ? "pausing" : "unpausing");
	}
	if (m_bServerChanged)
	{
		int index = 0;
		for (Servers::iterator it = g_pServerPool->GetServers()->begin(); it != g_pServerPool->GetServers()->end(); it++, index++)
		{
			NewsServer* pServer = *it;
			if (pServer->GetActive() != m_ServerStatusList[index])
			{
				info("Scheduler: %s %s", pServer->GetActive() ? "activating" : "deactivating", pServer->GetName());
			}
		}
		g_pServerPool->Changed();
	}
}

void Scheduler::EditServer(bool bActive, const char* szServerList)
{
	Tokenizer tok(szServerList, ",;");
	while (const char* szServer = tok.Next())
	{
		int iID = atoi(szServer);
		for (Servers::iterator it = g_pServerPool->GetServers()->begin(); it != g_pServerPool->GetServers()->end(); it++)
		{
			NewsServer* pServer = *it;
			if ((iID > 0 && pServer->GetID() == iID) ||
				!strcasecmp(pServer->GetName(), szServer))
			{
				if (!m_bServerChanged)
				{
					// store old server status for logging
					m_ServerStatusList.clear();
					m_ServerStatusList.reserve(g_pServerPool->GetServers()->size());
					for (Servers::iterator it2 = g_pServerPool->GetServers()->begin(); it2 != g_pServerPool->GetServers()->end(); it2++)
					{
						NewsServer* pServer2 = *it2;
						m_ServerStatusList.push_back(pServer2->GetActive());
					}
				}
				m_bServerChanged = true;
				pServer->SetActive(bActive);
				break;
			}
		}
	}
}

void Scheduler::FetchFeed(const char* szFeedList)
{
	Tokenizer tok(szFeedList, ",;");
	while (const char* szFeed = tok.Next())
	{
		int iID = atoi(szFeed);
		for (Feeds::iterator it = g_pFeedCoordinator->GetFeeds()->begin(); it != g_pFeedCoordinator->GetFeeds()->end(); it++)
		{
			FeedInfo* pFeed = *it;
			if (pFeed->GetID() == iID ||
				!strcasecmp(pFeed->GetName(), szFeed) ||
				!strcasecmp("0", szFeed))
			{
				g_pFeedCoordinator->FetchFeed(!strcasecmp("0", szFeed) ? 0 : pFeed->GetID());
				break;
			}
		}
	}
}

void Scheduler::CheckScheduledResume()
{
	time_t tResumeTime = g_pOptions->GetResumeTime();
	time_t tCurrentTime = time(NULL);
	if (tResumeTime > 0 && tCurrentTime >= tResumeTime)
	{
		info("Autoresume");
		g_pOptions->SetResumeTime(0);
		g_pOptions->SetPauseDownload(false);
		g_pOptions->SetPausePostProcess(false);
		g_pOptions->SetPauseScan(false);
	}
}


SchedulerScriptController::~SchedulerScriptController()
{
	free(m_szScript);
}

void SchedulerScriptController::StartScript(const char* szParam, bool bExternalProcess, int iTaskID)
{
	char** argv = NULL;
	if (bExternalProcess && !Util::SplitCommandLine(szParam, &argv))
	{
		error("Could not execute scheduled process-script, failed to parse command line: %s", szParam);
		return;
	}

	SchedulerScriptController* pScriptController = new SchedulerScriptController();

	pScriptController->m_bExternalProcess = bExternalProcess;
	pScriptController->m_szScript = strdup(szParam);
	pScriptController->m_iTaskID = iTaskID;

	if (bExternalProcess)
	{
		pScriptController->SetScript(argv[0]);
		pScriptController->SetArgs((const char**)argv, true);
	}

	pScriptController->SetAutoDestroy(true);

	pScriptController->Start();
}

void SchedulerScriptController::Run()
{
	if (m_bExternalProcess)
	{
		ExecuteExternalProcess();
	}
	else
	{
		ExecuteScriptList(m_szScript);
	}
}

void SchedulerScriptController::ExecuteScript(Options::Script* pScript)
{
	if (!pScript->GetSchedulerScript())
	{
		return;
	}

	PrintMessage(Message::mkInfo, "Executing scheduler-script %s for Task%i", pScript->GetName(), m_iTaskID);

	SetScript(pScript->GetLocation());
	SetArgs(NULL, false);

	char szInfoName[1024];
	snprintf(szInfoName, 1024, "scheduler-script %s for Task%i", pScript->GetName(), m_iTaskID);
	szInfoName[1024-1] = '\0';
	SetInfoName(szInfoName);

	SetLogPrefix(pScript->GetDisplayName());
	PrepareParams(pScript->GetName());

	Execute();

	SetLogPrefix(NULL);
}

void SchedulerScriptController::PrepareParams(const char* szScriptName)
{
	ResetEnv();

	SetIntEnvVar("NZBSP_TASKID", m_iTaskID);

	PrepareEnvScript(NULL, szScriptName);
}

void SchedulerScriptController::ExecuteExternalProcess()
{
	info("Executing scheduled process-script %s for Task%i", Util::BaseFileName(GetScript()), m_iTaskID);

	char szInfoName[1024];
	snprintf(szInfoName, 1024, "scheduled process-script %s for Task%i", Util::BaseFileName(GetScript()), m_iTaskID);
	szInfoName[1024-1] = '\0';
	SetInfoName(szInfoName);

	char szLogPrefix[1024];
	strncpy(szLogPrefix, Util::BaseFileName(GetScript()), 1024);
	szLogPrefix[1024-1] = '\0';
	if (char* ext = strrchr(szLogPrefix, '.')) *ext = '\0'; // strip file extension
	SetLogPrefix(szLogPrefix);

	Execute();
}
