/*
 *  This file is part of nzbget
 *
 *  Copyright (C) 2013-2015 Andrey Prygunkov <hugbug@users.sourceforge.net>
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
 * $Revision: 1231 $
 * $Date: 2015-03-13 22:14:52 +0100 (ven. 13 mars 2015) $
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
#ifndef WIN32
#include <unistd.h>
#endif
#include <errno.h>

#include "nzbget.h"
#include "Unpack.h"
#include "Log.h"
#include "Util.h"
#include "ParCoordinator.h"
#include "Options.h"

extern Options* g_pOptions;

void UnpackController::FileList::Clear()
{
	for (iterator it = begin(); it != end(); it++)
	{
		free(*it);
	}
	clear();
}

bool UnpackController::FileList::Exists(const char* szFilename)
{
	for (iterator it = begin(); it != end(); it++)
	{
		char* szFilename1 = *it;
		if (!strcmp(szFilename1, szFilename))
		{
			return true;
		}
	}

	return false;
}

UnpackController::ParamList::~ParamList()
{
	for (iterator it = begin(); it != end(); it++)
	{
		free(*it);
	}
}

bool UnpackController::ParamList::Exists(const char* szParam)
{
	for (iterator it = begin(); it != end(); it++)
	{
		char* szParam1 = *it;
		if (!strcmp(szParam1, szParam))
		{
			return true;
		}
	}

	return false;
}

void UnpackController::StartJob(PostInfo* pPostInfo)
{
	UnpackController* pUnpackController = new UnpackController();
	pUnpackController->m_pPostInfo = pPostInfo;
	pUnpackController->SetAutoDestroy(false);

	pPostInfo->SetPostThread(pUnpackController);

	pUnpackController->Start();
}

void UnpackController::Run()
{
	time_t tStart = time(NULL);

	// the locking is needed for accessing the members of NZBInfo
	DownloadQueue::Lock();

	strncpy(m_szDestDir, m_pPostInfo->GetNZBInfo()->GetDestDir(), 1024);
	m_szDestDir[1024-1] = '\0';

	strncpy(m_szName, m_pPostInfo->GetNZBInfo()->GetName(), 1024);
	m_szName[1024-1] = '\0';

	m_bCleanedUpDisk = false;
	m_szPassword[0] = '\0';
	m_szFinalDir[0] = '\0';
	m_bFinalDirCreated = false;
	m_bUnpackOK = true;
	m_bUnpackStartError = false;
	m_bUnpackSpaceError = false;
	m_bUnpackDecryptError = false;
	m_bUnpackPasswordError = false;
	m_bAutoTerminated = false;
	m_bPassListTried = false;

	NZBParameter* pParameter = m_pPostInfo->GetNZBInfo()->GetParameters()->Find("*Unpack:", false);
	bool bUnpack = !(pParameter && !strcasecmp(pParameter->GetValue(), "no"));

	pParameter = m_pPostInfo->GetNZBInfo()->GetParameters()->Find("*Unpack:Password", false);
	if (pParameter)
	{
		strncpy(m_szPassword, pParameter->GetValue(), 1024-1);
		m_szPassword[1024-1] = '\0';
	}
	
	DownloadQueue::Unlock();

	snprintf(m_szInfoName, 1024, "unpack for %s", m_szName);
	m_szInfoName[1024-1] = '\0';

	snprintf(m_szInfoNameUp, 1024, "Unpack for %s", m_szName); // first letter in upper case
	m_szInfoNameUp[1024-1] = '\0';

	m_bHasParFiles = ParCoordinator::FindMainPars(m_szDestDir, NULL);

	if (bUnpack)
	{
		bool bScanNonStdFiles = m_pPostInfo->GetNZBInfo()->GetRenameStatus() > NZBInfo::rsSkipped ||
			m_pPostInfo->GetNZBInfo()->GetParStatus() == NZBInfo::psSuccess ||
			!m_bHasParFiles;
		CheckArchiveFiles(bScanNonStdFiles);
	}

	SetInfoName(m_szInfoName);
	SetWorkingDir(m_szDestDir);

	bool bHasFiles = m_bHasRarFiles || m_bHasNonStdRarFiles || m_bHasSevenZipFiles || m_bHasSevenZipMultiFiles || m_bHasSplittedFiles;

	if (m_pPostInfo->GetUnpackTried() && !m_pPostInfo->GetParRepaired() &&
		(!Util::EmptyStr(m_szPassword) || Util::EmptyStr(g_pOptions->GetUnpackPassFile()) || m_pPostInfo->GetPassListTried()))
	{
		PrintMessage(Message::mkError, "%s failed: second unpack attempt skipped due to par-check not repaired anything", m_szInfoNameUp);
		m_pPostInfo->GetNZBInfo()->SetUnpackStatus((NZBInfo::EUnpackStatus)m_pPostInfo->GetLastUnpackStatus());
		m_pPostInfo->SetStage(PostInfo::ptQueued);
	}
	else if (bUnpack && bHasFiles)
	{
		PrintMessage(Message::mkInfo, "Unpacking %s", m_szName);

		CreateUnpackDir();

		if (m_bHasRarFiles || m_bHasNonStdRarFiles)
		{
			UnpackArchives(upUnrar, false);
		}

		if (m_bHasSevenZipFiles && m_bUnpackOK)
		{
			UnpackArchives(upSevenZip, false);
		}

		if (m_bHasSevenZipMultiFiles && m_bUnpackOK)
		{
			UnpackArchives(upSevenZip, true);
		}

		if (m_bHasSplittedFiles && m_bUnpackOK)
		{
			JoinSplittedFiles();
		}

		Completed();

		m_JoinedFiles.Clear();
	}
	else
	{
		PrintMessage(Message::mkInfo, (bUnpack ? "Nothing to unpack for %s" : "Unpack for %s skipped"), m_szName);

#ifndef DISABLE_PARCHECK
		if (bUnpack && m_pPostInfo->GetNZBInfo()->GetParStatus() <= NZBInfo::psSkipped && 
			m_pPostInfo->GetNZBInfo()->GetRenameStatus() <= NZBInfo::rsSkipped && m_bHasParFiles)
		{
			RequestParCheck(false);
		}
		else
#endif
		{
			m_pPostInfo->GetNZBInfo()->SetUnpackStatus(NZBInfo::usSkipped);
			m_pPostInfo->SetStage(PostInfo::ptQueued);
		}
	}

	int iUnpackSec = (int)(time(NULL) - tStart);
	m_pPostInfo->GetNZBInfo()->SetUnpackSec(m_pPostInfo->GetNZBInfo()->GetUnpackSec() + iUnpackSec);

	m_pPostInfo->SetWorking(false);
}

void UnpackController::UnpackArchives(EUnpacker eUnpacker, bool bMultiVolumes)
{
	if (!m_pPostInfo->GetUnpackTried() || m_pPostInfo->GetParRepaired())
	{
		ExecuteUnpack(eUnpacker, m_szPassword, bMultiVolumes);
		if (!m_bUnpackOK && m_bHasParFiles && !m_bUnpackPasswordError &&
			m_pPostInfo->GetNZBInfo()->GetParStatus() <= NZBInfo::psSkipped)
		{
			// for rar4- or 7z-archives try par-check first, before trying password file
			return;
		}
	}
	else
	{
		m_bUnpackOK = false;
		m_bUnpackDecryptError = m_pPostInfo->GetLastUnpackStatus() == (int)NZBInfo::usPassword;
	}

	if (!m_bUnpackOK && !m_bUnpackStartError && !m_bUnpackSpaceError &&
		(m_bUnpackDecryptError || m_bUnpackPasswordError) &&
		(!GetTerminated() || m_bAutoTerminated) &&
		Util::EmptyStr(m_szPassword) && !Util::EmptyStr(g_pOptions->GetUnpackPassFile()))
	{
		FILE* infile = fopen(g_pOptions->GetUnpackPassFile(), FOPEN_RB);
		if (!infile)
		{
			PrintMessage(Message::mkError, "Could not open file %s", g_pOptions->GetUnpackPassFile());
			return;
		}

		char szPassword[512];
		while (!m_bUnpackOK && !m_bUnpackStartError && !m_bUnpackSpaceError &&
			(m_bUnpackDecryptError || m_bUnpackPasswordError) &&
			fgets(szPassword, sizeof(szPassword) - 1, infile))
		{
			// trim trailing <CR> and <LF>
			char* szEnd = szPassword + strlen(szPassword) - 1;
			while (szEnd >= szPassword && (*szEnd == '\n' || *szEnd == '\r')) *szEnd-- = '\0';

			if (!Util::EmptyStr(szPassword))
			{
				if (IsStopped() && m_bAutoTerminated)
				{
					ScriptController::Resume();
					Thread::Resume();
				}
				m_bUnpackDecryptError = false;
				m_bUnpackPasswordError = false;
				m_bAutoTerminated = false;
				PrintMessage(Message::mkInfo, "Trying password %s for %s", szPassword, m_szName);
				ExecuteUnpack(eUnpacker, szPassword, bMultiVolumes);
			}
		}

		fclose(infile);
		m_bPassListTried = !IsStopped() || m_bAutoTerminated;
	}
}

void UnpackController::ExecuteUnpack(EUnpacker eUnpacker, const char* szPassword, bool bMultiVolumes)
{
	switch (eUnpacker)
	{
		case upUnrar:
			ExecuteUnrar(szPassword);
			break;

		case upSevenZip:
			ExecuteSevenZip(szPassword, bMultiVolumes);
			break;
	}
}

void UnpackController::ExecuteUnrar(const char* szPassword)
{
	// Format: 
	//   unrar x -y -p- -o+ *.rar ./_unpack/

	ParamList params;
	if (!PrepareCmdParams(g_pOptions->GetUnrarCmd(), &params, "unrar"))
	{
		return;
	}

	if (!params.Exists("x") && !params.Exists("e"))
	{
		params.push_back(strdup("x"));
	}

	params.push_back(strdup("-y"));

	if (!Util::EmptyStr(szPassword))
	{
		char szPasswordParam[1024];
		snprintf(szPasswordParam, 1024, "-p%s", szPassword);
		szPasswordParam[1024-1] = '\0';
		params.push_back(strdup(szPasswordParam));
	}
	else
	{
		params.push_back(strdup("-p-"));
	}

	if (!params.Exists("-o+") && !params.Exists("-o-"))
	{
		params.push_back(strdup("-o+"));
	}

	params.push_back(strdup(m_bHasNonStdRarFiles ? "*.*" : "*.rar"));

	char szUnpackDirParam[1024];
	snprintf(szUnpackDirParam, 1024, "%s%c", m_szUnpackDir, PATH_SEPARATOR);
	szUnpackDirParam[1024-1] = '\0';
	params.push_back(strdup(szUnpackDirParam));

	params.push_back(NULL);
	SetArgs((const char**)&params.front(), false);
	SetScript(params.at(0));
	SetLogPrefix("Unrar");
	ResetEnv();

	m_bAllOKMessageReceived = false;
	m_eUnpacker = upUnrar;

	SetProgressLabel("");
	int iExitCode = Execute();
	SetLogPrefix(NULL);
	SetProgressLabel("");

	m_bUnpackOK = iExitCode == 0 && m_bAllOKMessageReceived && !GetTerminated();
	m_bUnpackStartError = iExitCode == -1;
	m_bUnpackSpaceError = iExitCode == 5;
	m_bUnpackPasswordError |= iExitCode == 11; // only for rar5-archives

	if (!m_bUnpackOK && iExitCode > 0)
	{
		PrintMessage(Message::mkError, "Unrar error code: %i", iExitCode);
	}
}

void UnpackController::ExecuteSevenZip(const char* szPassword, bool bMultiVolumes)
{
	// Format: 
	//   7z x -y -p- -o./_unpack *.7z
	// OR
	//   7z x -y -p- -o./_unpack *.7z.001

	ParamList params;
	if (!PrepareCmdParams(g_pOptions->GetSevenZipCmd(), &params, "7-Zip"))
	{
		return;
	}

	if (!params.Exists("x") && !params.Exists("e"))
	{
		params.push_back(strdup("x"));
	}

	params.push_back(strdup("-y"));

	if (!Util::EmptyStr(szPassword))
	{
		char szPasswordParam[1024];
		snprintf(szPasswordParam, 1024, "-p%s", szPassword);
		szPasswordParam[1024-1] = '\0';
		params.push_back(strdup(szPasswordParam));
	}
	else
	{
		params.push_back(strdup("-p-"));
	}

	char szUnpackDirParam[1024];
	snprintf(szUnpackDirParam, 1024, "-o%s", m_szUnpackDir);
	szUnpackDirParam[1024-1] = '\0';
	params.push_back(strdup(szUnpackDirParam));

	params.push_back(strdup(bMultiVolumes ? "*.7z.001" : "*.7z"));

	params.push_back(NULL);
	SetArgs((const char**)&params.front(), false);
	SetScript(params.at(0));
	ResetEnv();

	m_bAllOKMessageReceived = false;
	m_eUnpacker = upSevenZip;

	PrintMessage(Message::mkInfo, "Executing 7-Zip");
	SetLogPrefix("7-Zip");
	SetProgressLabel("");
	int iExitCode = Execute();
	SetLogPrefix(NULL);
	SetProgressLabel("");

	m_bUnpackOK = iExitCode == 0 && m_bAllOKMessageReceived && !GetTerminated();
	m_bUnpackStartError = iExitCode == -1;

	if (!m_bUnpackOK && iExitCode > 0)
	{
		PrintMessage(Message::mkError, "7-Zip error code: %i", iExitCode);
	}
}

bool UnpackController::PrepareCmdParams(const char* szCommand, ParamList* pParams, const char* szInfoName)
{
	if (Util::FileExists(szCommand))
	{
		pParams->push_back(strdup(szCommand));
		return true;
	}

	char** pCmdArgs = NULL;
	if (!Util::SplitCommandLine(szCommand, &pCmdArgs))
	{
		PrintMessage(Message::mkError, "Could not start %s, failed to parse command line: %s", szInfoName, szCommand);
		m_bUnpackOK = false;
		m_bUnpackStartError = true;
		return false;
	}

	for (char** szArgPtr = pCmdArgs; *szArgPtr; szArgPtr++)
	{
		pParams->push_back(*szArgPtr);
	}
	free(pCmdArgs);

	return true;
}

void UnpackController::JoinSplittedFiles()
{
	SetLogPrefix("Join");
	SetProgressLabel("");
	m_pPostInfo->SetStageProgress(0);

	// determine groups

	FileList groups;
	RegEx regExSplitExt(".*\\.[a-z,0-9]{3}\\.001$");

	DirBrowser dir(m_szDestDir);
	while (const char* filename = dir.Next())
	{
		char szFullFilename[1024];
		snprintf(szFullFilename, 1024, "%s%c%s", m_szDestDir, PATH_SEPARATOR, filename);
		szFullFilename[1024-1] = '\0';

		if (strcmp(filename, ".") && strcmp(filename, "..") && !Util::DirectoryExists(szFullFilename))
		{
			if (regExSplitExt.Match(filename) && !FileHasRarSignature(szFullFilename))
			{
				if (!JoinFile(filename))
				{
					m_bUnpackOK = false;
					break;
				}
			}
		}
	}

	SetLogPrefix(NULL);
	SetProgressLabel("");
}

bool UnpackController::JoinFile(const char* szFragBaseName)
{
	char szDestBaseName[1024];
	strncpy(szDestBaseName, szFragBaseName, 1024);
	szDestBaseName[1024-1] = '\0';

	// trim extension
	char* szExtension = strrchr(szDestBaseName, '.');
	*szExtension = '\0';

	char szFullFilename[1024];
	snprintf(szFullFilename, 1024, "%s%c%s", m_szDestDir, PATH_SEPARATOR, szFragBaseName);
	szFullFilename[1024-1] = '\0';
	long long lFirstSegmentSize = Util::FileSize(szFullFilename);
	long long lDifSegmentSize = 0;

	// Validate joinable file:
	//  - fragments have continuous numbers (no holes);
	//  - fragments have the same size (except of the last fragment);
	//  - the last fragment must be smaller than other fragments,
	//  if it has the same size it is probably not the last and there are missing fragments.

	RegEx regExSplitExt(".*\\.[a-z,0-9]{3}\\.[0-9]{3}$");
	int iCount = 0;
	int iMin = -1;
	int iMax = -1;
	int iDifSizeCount = 0;
	int iDifSizeMin = 999999;
	DirBrowser dir(m_szDestDir);
	while (const char* filename = dir.Next())
	{
		snprintf(szFullFilename, 1024, "%s%c%s", m_szDestDir, PATH_SEPARATOR, filename);
		szFullFilename[1024-1] = '\0';

		if (strcmp(filename, ".") && strcmp(filename, "..") && !Util::DirectoryExists(szFullFilename) &&
			regExSplitExt.Match(filename))
		{
			const char* szSegExt = strrchr(filename, '.');
			int iSegNum = atoi(szSegExt + 1);
			iCount++;
			iMin = iSegNum < iMin || iMin == -1 ? iSegNum : iMin;
			iMax = iSegNum > iMax ? iSegNum : iMax;
			
			long long lSegmentSize = Util::FileSize(szFullFilename);
			if (lSegmentSize != lFirstSegmentSize)
			{
				iDifSizeCount++;
				iDifSizeMin = iSegNum < iDifSizeMin ? iSegNum : iDifSizeMin;
				lDifSegmentSize = lSegmentSize;
			}
		}
	}

	int iCorrectedCount = iCount - (iMin == 0 ? 1 : 0);
	if ((iMin > 1) || iCorrectedCount != iMax ||
		((iDifSizeMin != iCorrectedCount || iDifSizeMin > iMax) &&
		 m_pPostInfo->GetNZBInfo()->GetParStatus() != NZBInfo::psSuccess))
	{
		PrintMessage(Message::mkWarning, "Could not join splitted file %s: missing fragments detected", szDestBaseName);
		return false;
	}

	// Now can join
	PrintMessage(Message::mkInfo, "Joining splitted file %s", szDestBaseName);
	m_pPostInfo->SetStageProgress(0);

	char szErrBuf[256];
	char szDestFilename[1024];
	snprintf(szDestFilename, 1024, "%s%c%s", m_szUnpackDir, PATH_SEPARATOR, szDestBaseName);
	szDestFilename[1024-1] = '\0';

	FILE* pOutFile = fopen(szDestFilename, FOPEN_WBP);
	if (!pOutFile)
	{
		PrintMessage(Message::mkError, "Could not create file %s: %s", szDestFilename, Util::GetLastErrorMessage(szErrBuf, sizeof(szErrBuf)));
		return false;
	}
	if (g_pOptions->GetWriteBuffer() > 0)
	{
		setvbuf(pOutFile, NULL, _IOFBF, g_pOptions->GetWriteBuffer() * 1024);
	}

	long long lTotalSize = lFirstSegmentSize * (iCount - 1) + lDifSegmentSize;
	long long lWritten = 0;

	static const int BUFFER_SIZE = 1024 * 50;
	char* buffer = (char*)malloc(BUFFER_SIZE);

	bool bOK = true;
	for (int i = iMin; i <= iMax; i++)
	{
		PrintMessage(Message::mkInfo, "Joining from %s.%.3i", szDestBaseName, i);

		char szMessage[1024];
		snprintf(szMessage, 1024, "Joining from %s.%.3i", szDestBaseName, i);
		szMessage[1024-1] = '\0';
		SetProgressLabel(szMessage);

		char szFragFilename[1024];
		snprintf(szFragFilename, 1024, "%s%c%s.%.3i", m_szDestDir, PATH_SEPARATOR, szDestBaseName, i);
		szFragFilename[1024-1] = '\0';
		if (!Util::FileExists(szFragFilename))
		{
			break;
		}

		FILE* pInFile = fopen(szFragFilename, FOPEN_RB);
		if (pInFile)
		{
			int cnt = BUFFER_SIZE;
			while (cnt == BUFFER_SIZE)
			{
				cnt = (int)fread(buffer, 1, BUFFER_SIZE, pInFile);
				fwrite(buffer, 1, cnt, pOutFile);
				lWritten += cnt;
				m_pPostInfo->SetStageProgress(int(lWritten * 1000 / lTotalSize));
			}
			fclose(pInFile);

			char szFragFilename[1024];
			snprintf(szFragFilename, 1024, "%s.%.3i", szDestBaseName, i);
			szFragFilename[1024-1] = '\0';
			m_JoinedFiles.push_back(strdup(szFragFilename));
		}
		else
		{
			PrintMessage(Message::mkError, "Could not open file %s", szFragFilename);
			bOK = false;
			break;
		}
	}

	fclose(pOutFile);
	free(buffer);

	return bOK;
}

void UnpackController::Completed()
{
	bool bCleanupSuccess = Cleanup();

	if (m_bUnpackOK && bCleanupSuccess)
	{
		PrintMessage(Message::mkInfo, "%s %s", m_szInfoNameUp, "successful");
		m_pPostInfo->GetNZBInfo()->SetUnpackStatus(NZBInfo::usSuccess);
		m_pPostInfo->GetNZBInfo()->SetUnpackCleanedUpDisk(m_bCleanedUpDisk);
		if (g_pOptions->GetParRename())
		{
			//request par-rename check for extracted files
			m_pPostInfo->GetNZBInfo()->SetRenameStatus(NZBInfo::rsNone);
		}
		m_pPostInfo->SetStage(PostInfo::ptQueued);
	}
	else
	{
#ifndef DISABLE_PARCHECK
		if (!m_bUnpackOK && 
			(m_pPostInfo->GetNZBInfo()->GetParStatus() <= NZBInfo::psSkipped ||
			 !m_pPostInfo->GetNZBInfo()->GetParFull()) &&
			!m_bUnpackStartError && !m_bUnpackSpaceError && !m_bUnpackPasswordError &&
			(!GetTerminated() || m_bAutoTerminated) && m_bHasParFiles)
		{
			RequestParCheck(!Util::EmptyStr(m_szPassword) ||
				Util::EmptyStr(g_pOptions->GetUnpackPassFile()) || m_bPassListTried ||
				!(m_bUnpackDecryptError || m_bUnpackPasswordError) ||
				m_pPostInfo->GetNZBInfo()->GetParStatus() > NZBInfo::psSkipped);
		}
		else
#endif
		{
			PrintMessage(Message::mkError, "%s failed", m_szInfoNameUp);
			m_pPostInfo->GetNZBInfo()->SetUnpackStatus(
				m_bUnpackSpaceError ? NZBInfo::usSpace :
				m_bUnpackPasswordError || m_bUnpackDecryptError ? NZBInfo::usPassword :
				NZBInfo::usFailure);
			m_pPostInfo->SetStage(PostInfo::ptQueued);
		}
	}
}

#ifndef DISABLE_PARCHECK
void UnpackController::RequestParCheck(bool bForceRepair)
{
	PrintMessage(Message::mkInfo, "%s requested %s", m_szInfoNameUp, bForceRepair ? "par-check with forced repair" : "par-check/repair");
	m_pPostInfo->SetRequestParCheck(true);
	m_pPostInfo->SetForceRepair(bForceRepair);
	m_pPostInfo->SetStage(PostInfo::ptFinished);
	m_pPostInfo->SetUnpackTried(true);
	m_pPostInfo->SetPassListTried(m_bPassListTried);
	m_pPostInfo->SetLastUnpackStatus((int)(m_bUnpackSpaceError ? NZBInfo::usSpace :
		m_bUnpackPasswordError || m_bUnpackDecryptError ? NZBInfo::usPassword :
		NZBInfo::usFailure));
}
#endif

void UnpackController::CreateUnpackDir()
{
	m_bInterDir = strlen(g_pOptions->GetInterDir()) > 0 &&
		!strncmp(m_szDestDir, g_pOptions->GetInterDir(), strlen(g_pOptions->GetInterDir()));
	if (m_bInterDir)
	{
		m_pPostInfo->GetNZBInfo()->BuildFinalDirName(m_szFinalDir, 1024);
		m_szFinalDir[1024-1] = '\0';
		snprintf(m_szUnpackDir, 1024, "%s%c%s", m_szFinalDir, PATH_SEPARATOR, "_unpack");
		m_bFinalDirCreated = !Util::DirectoryExists(m_szFinalDir);
	}
	else
	{
		snprintf(m_szUnpackDir, 1024, "%s%c%s", m_szDestDir, PATH_SEPARATOR, "_unpack");
	}
	m_szUnpackDir[1024-1] = '\0';

	char szErrBuf[1024];
	if (!Util::ForceDirectories(m_szUnpackDir, szErrBuf, sizeof(szErrBuf)))
	{
		PrintMessage(Message::mkError, "Could not create directory %s: %s", m_szUnpackDir, szErrBuf);
	}
}


void UnpackController::CheckArchiveFiles(bool bScanNonStdFiles)
{
	m_bHasRarFiles = false;
	m_bHasNonStdRarFiles = false;
	m_bHasSevenZipFiles = false;
	m_bHasSevenZipMultiFiles = false;
	m_bHasSplittedFiles = false;

	RegEx regExRar(".*\\.rar$");
	RegEx regExRarMultiSeq(".*\\.(r|s)[0-9][0-9]$");
	RegEx regExSevenZip(".*\\.7z$");
	RegEx regExSevenZipMulti(".*\\.7z\\.[0-9]+$");
	RegEx regExNumExt(".*\\.[0-9]+$");
	RegEx regExSplitExt(".*\\.[a-z,0-9]{3}\\.[0-9]{3}$");

	DirBrowser dir(m_szDestDir);
	while (const char* filename = dir.Next())
	{
		char szFullFilename[1024];
		snprintf(szFullFilename, 1024, "%s%c%s", m_szDestDir, PATH_SEPARATOR, filename);
		szFullFilename[1024-1] = '\0';

		if (strcmp(filename, ".") && strcmp(filename, "..") && !Util::DirectoryExists(szFullFilename))
		{
			const char* szExt = strchr(filename, '.');
			int iExtNum = szExt ? atoi(szExt + 1) : -1;

			if (regExRar.Match(filename))
			{
				m_bHasRarFiles = true;
			}
			else if (regExSevenZip.Match(filename))
			{
				m_bHasSevenZipFiles = true;
			}
			else if (regExSevenZipMulti.Match(filename))
			{
				m_bHasSevenZipMultiFiles = true;
			}
			else if (bScanNonStdFiles && !m_bHasNonStdRarFiles && iExtNum > 1 &&
				!regExRarMultiSeq.Match(filename) && regExNumExt.Match(filename) &&
				FileHasRarSignature(szFullFilename))
			{
				m_bHasNonStdRarFiles = true;
			}
			else if (regExSplitExt.Match(filename) && (iExtNum == 0 || iExtNum == 1))
			{
				m_bHasSplittedFiles = true;
			}
		}
	}
}

bool UnpackController::FileHasRarSignature(const char* szFilename)
{
	char rar4Signature[] = { 0x52, 0x61, 0x72, 0x21, 0x1A, 0x07, 0x00 };
	char rar5Signature[] = { 0x52, 0x61, 0x72, 0x21, 0x1A, 0x07, 0x01, 0x00 };

	char fileSignature[8];

	int cnt = 0;
	FILE* infile;
	infile = fopen(szFilename, FOPEN_RB);
	if (infile)
	{
		cnt = (int)fread(fileSignature, 1, sizeof(fileSignature), infile);
		fclose(infile);
	}

	bool bRar = cnt == sizeof(fileSignature) && 
		(!strcmp(rar4Signature, fileSignature) || !strcmp(rar5Signature, fileSignature));
	return bRar;
}

bool UnpackController::Cleanup()
{
	// By success:
	//   - move unpacked files to destination dir;
	//   - remove _unpack-dir;
	//   - delete archive-files.
	// By failure:
	//   - remove _unpack-dir.

	bool bOK = true;

	FileList extractedFiles;

	if (m_bUnpackOK)
	{
		// moving files back
		DirBrowser dir(m_szUnpackDir);
		while (const char* filename = dir.Next())
		{
			if (strcmp(filename, ".") && strcmp(filename, ".."))
			{
				char szSrcFile[1024];
				snprintf(szSrcFile, 1024, "%s%c%s", m_szUnpackDir, PATH_SEPARATOR, filename);
				szSrcFile[1024-1] = '\0';

				char szDstFile[1024];
				snprintf(szDstFile, 1024, "%s%c%s", m_szFinalDir[0] != '\0' ? m_szFinalDir : m_szDestDir, PATH_SEPARATOR, filename);
				szDstFile[1024-1] = '\0';

				// silently overwrite existing files
				remove(szDstFile);

				bool bHiddenFile = filename[0] == '.';

				if (!Util::MoveFile(szSrcFile, szDstFile) && !bHiddenFile)
				{
					char szErrBuf[256];
					PrintMessage(Message::mkError, "Could not move file %s to %s: %s", szSrcFile, szDstFile,
						Util::GetLastErrorMessage(szErrBuf, sizeof(szErrBuf)));
					bOK = false;
				}

				extractedFiles.push_back(strdup(filename));
			}
		}
	}

	char szErrBuf[256];
	if (bOK && !Util::DeleteDirectoryWithContent(m_szUnpackDir, szErrBuf, sizeof(szErrBuf)))
	{
		PrintMessage(Message::mkError, "Could not delete temporary directory %s: %s", m_szUnpackDir, szErrBuf);
	}

	if (!m_bUnpackOK && m_bFinalDirCreated)
	{
		Util::RemoveDirectory(m_szFinalDir);
	}		

	if (m_bUnpackOK && bOK && g_pOptions->GetUnpackCleanupDisk())
	{
		PrintMessage(Message::mkInfo, "Deleting archive files");

		RegEx regExRar(".*\\.rar$");
		RegEx regExRarMultiSeq(".*\\.[r-z][0-9][0-9]$");
		RegEx regExSevenZip(".*\\.7z$|.*\\.7z\\.[0-9]+$");
		RegEx regExNumExt(".*\\.[0-9]+$");
		RegEx regExSplitExt(".*\\.[a-z,0-9]{3}\\.[0-9]{3}$");

		DirBrowser dir(m_szDestDir);
		while (const char* filename = dir.Next())
		{
			char szFullFilename[1024];
			snprintf(szFullFilename, 1024, "%s%c%s", m_szDestDir, PATH_SEPARATOR, filename);
			szFullFilename[1024-1] = '\0';

			if (strcmp(filename, ".") && strcmp(filename, "..") &&
				!Util::DirectoryExists(szFullFilename) &&
				(m_bInterDir || !extractedFiles.Exists(filename)) &&
				(regExRar.Match(filename) || regExSevenZip.Match(filename) ||
				 (regExRarMultiSeq.Match(filename) && FileHasRarSignature(szFullFilename)) ||
				 (m_bHasNonStdRarFiles && regExNumExt.Match(filename) && FileHasRarSignature(szFullFilename)) ||
				 (m_bHasSplittedFiles && regExSplitExt.Match(filename) && m_JoinedFiles.Exists(filename))))
			{
				PrintMessage(Message::mkInfo, "Deleting file %s", filename);

				if (remove(szFullFilename) != 0)
				{
					char szErrBuf[256];
					PrintMessage(Message::mkError, "Could not delete file %s: %s", szFullFilename, Util::GetLastErrorMessage(szErrBuf, sizeof(szErrBuf)));
				}
			}
		}

		m_bCleanedUpDisk = true;
	}

	extractedFiles.Clear();

	return bOK;
}

/**
 * Unrar prints progress information into the same line using backspace control character.
 * In order to print progress continuously we analyze the output after every char
 * and update post-job progress information.
 */
bool UnpackController::ReadLine(char* szBuf, int iBufSize, FILE* pStream)
{
	bool bPrinted = false;

	int i = 0;

	for (; i < iBufSize - 1; i++)
	{
		int ch = fgetc(pStream);
		szBuf[i] = ch;
		szBuf[i+1] = '\0';
		if (ch == EOF)
		{
			break;
		}
		if (ch == '\n')
		{
			i++;
			break;
		}

		char* szBackspace = strrchr(szBuf, '\b');
		if (szBackspace)
		{
			if (!bPrinted)
			{
				char tmp[1024];
				strncpy(tmp, szBuf, 1024);
				tmp[1024-1] = '\0';
				char* szTmpPercent = strrchr(tmp, '\b');
				if (szTmpPercent)
				{
					*szTmpPercent = '\0';
				}
				if (strncmp(szBuf, "...", 3))
				{
					ProcessOutput(tmp);
				}
				bPrinted = true;
			}
			if (strchr(szBackspace, '%'))
			{
				int iPercent = atoi(szBackspace + 1);
				m_pPostInfo->SetStageProgress(iPercent * 10);
			}
		}
	}

	szBuf[i] = '\0';

	if (bPrinted)
	{
		szBuf[0] = '\0';
	}

	return i > 0;
}

void UnpackController::AddMessage(Message::EKind eKind, const char* szText)
{
	char szMsgText[1024];
	strncpy(szMsgText, szText, 1024);
	szMsgText[1024-1] = '\0';
	int iLen = strlen(szText);
	
	// Modify unrar messages for better readability:
	// remove the destination path part from message "Extracting file.xxx"
	if (m_eUnpacker == upUnrar && !strncmp(szText, "Unrar: Extracting  ", 19) &&
		!strncmp(szText + 19, m_szUnpackDir, strlen(m_szUnpackDir)))
	{
		snprintf(szMsgText, 1024, "Unrar: Extracting %s", szText + 19 + strlen(m_szUnpackDir) + 1);
		szMsgText[1024-1] = '\0';
	}

	m_pPostInfo->GetNZBInfo()->AddMessage(eKind, szMsgText);

	if (m_eUnpacker == upUnrar && !strncmp(szMsgText, "Unrar: UNRAR ", 6) &&
		strstr(szMsgText, " Copyright ") && strstr(szMsgText, " Alexander Roshal"))
	{
		// reset start time for a case if user uses unpack-script to do some things
		// (like sending Wake-On-Lan message) before executing unrar
		m_pPostInfo->SetStageTime(time(NULL));
	}

	if (m_eUnpacker == upUnrar && !strncmp(szMsgText, "Unrar: Extracting ", 18))
	{
		SetProgressLabel(szMsgText + 7);
	}

	if (m_eUnpacker == upUnrar && !strncmp(szText, "Unrar: Extracting from ", 23))
	{
		const char *szFilename = szText + 23;
		debug("Filename: %s", szFilename);
		SetProgressLabel(szText + 7);
	}

	if (m_eUnpacker == upUnrar &&
		(!strncmp(szText, "Unrar: Checksum error in the encrypted file", 42) ||
		 !strncmp(szText, "Unrar: CRC failed in the encrypted file", 39)))
	{
		m_bUnpackDecryptError = true;
	}

	if (m_eUnpacker == upUnrar && !strncmp(szText, "Unrar: The specified password is incorrect.'", 43))
	{
		m_bUnpackPasswordError = true;
	}

	if (m_eUnpacker == upSevenZip &&
		(iLen > 18 && !strncmp(szText + iLen - 45, "Data Error in encrypted file. Wrong password?", 45)))
	{
		m_bUnpackDecryptError = true;
	}

	if (!IsStopped() && (m_bUnpackDecryptError || m_bUnpackPasswordError ||
		strstr(szText, " : packed data CRC failed in volume") ||
		strstr(szText, " : packed data checksum error in volume") ||
		(iLen > 13 && !strncmp(szText + iLen - 13, " - CRC failed", 13)) ||
		(iLen > 18 && !strncmp(szText + iLen - 18, " - checksum failed", 18)) ||
		!strncmp(szText, "Unrar: WARNING: You need to start extraction from a previous volume", 67)))
	{
		char szMsgText[1024];
		snprintf(szMsgText, 1024, "Cancelling %s due to errors", m_szInfoName);
		szMsgText[1024-1] = '\0';
		m_pPostInfo->GetNZBInfo()->AddMessage(Message::mkWarning, szMsgText);
		m_bAutoTerminated = true;
		Stop();
	}

	if ((m_eUnpacker == upUnrar && !strncmp(szText, "Unrar: All OK", 13)) ||
		(m_eUnpacker == upSevenZip && !strncmp(szText, "7-Zip: Everything is Ok", 23)))
	{
		m_bAllOKMessageReceived = true;
	}
}

void UnpackController::Stop()
{
	debug("Stopping unpack");
	Thread::Stop();
	Terminate();
}

void UnpackController::SetProgressLabel(const char* szProgressLabel)
{
	DownloadQueue::Lock();
	m_pPostInfo->SetProgressLabel(szProgressLabel);
	DownloadQueue::Unlock();
}


void MoveController::StartJob(PostInfo* pPostInfo)
{
	MoveController* pMoveController = new MoveController();
	pMoveController->m_pPostInfo = pPostInfo;
	pMoveController->SetAutoDestroy(false);

	pPostInfo->SetPostThread(pMoveController);

	pMoveController->Start();
}

void MoveController::Run()
{
	// the locking is needed for accessing the members of NZBInfo
	DownloadQueue::Lock();

	char szNZBName[1024];
	strncpy(szNZBName, m_pPostInfo->GetNZBInfo()->GetName(), 1024);
	szNZBName[1024-1] = '\0';

	char szInfoName[1024];
	snprintf(szInfoName, 1024, "move for %s", m_pPostInfo->GetNZBInfo()->GetName());
	szInfoName[1024-1] = '\0';
	SetInfoName(szInfoName);

	strncpy(m_szInterDir, m_pPostInfo->GetNZBInfo()->GetDestDir(), 1024);
	m_szInterDir[1024-1] = '\0';

	m_pPostInfo->GetNZBInfo()->BuildFinalDirName(m_szDestDir, 1024);
	m_szDestDir[1024-1] = '\0';

	DownloadQueue::Unlock();

	PrintMessage(Message::mkInfo, "Moving completed files for %s", szNZBName);

	bool bOK = MoveFiles();

	szInfoName[0] = 'M'; // uppercase

	if (bOK)
	{
		PrintMessage(Message::mkInfo, "%s successful", szInfoName);
		// save new dest dir
		DownloadQueue::Lock();
		m_pPostInfo->GetNZBInfo()->SetDestDir(m_szDestDir);
		m_pPostInfo->GetNZBInfo()->SetMoveStatus(NZBInfo::msSuccess);
		DownloadQueue::Unlock();
	}
	else
	{
		PrintMessage(Message::mkError, "%s failed", szInfoName);
		m_pPostInfo->GetNZBInfo()->SetMoveStatus(NZBInfo::msFailure);
	}

	m_pPostInfo->SetStage(PostInfo::ptQueued);
	m_pPostInfo->SetWorking(false);
}

bool MoveController::MoveFiles()
{
	char szErrBuf[1024];
	if (!Util::ForceDirectories(m_szDestDir, szErrBuf, sizeof(szErrBuf)))
	{
		PrintMessage(Message::mkError, "Could not create directory %s: %s", m_szDestDir, szErrBuf);
		return false;
	}

	bool bOK = true;
	DirBrowser dir(m_szInterDir);
	while (const char* filename = dir.Next())
	{
		if (strcmp(filename, ".") && strcmp(filename, ".."))
		{
			char szSrcFile[1024];
			snprintf(szSrcFile, 1024, "%s%c%s", m_szInterDir, PATH_SEPARATOR, filename);
			szSrcFile[1024-1] = '\0';

			char szDstFile[1024];
			Util::MakeUniqueFilename(szDstFile, 1024, m_szDestDir, filename);

			bool bHiddenFile = filename[0] == '.';

			if (!bHiddenFile)
			{
				PrintMessage(Message::mkInfo, "Moving file %s to %s", Util::BaseFileName(szSrcFile), m_szDestDir);
			}

			if (!Util::MoveFile(szSrcFile, szDstFile) && !bHiddenFile)
			{
				char szErrBuf[256];
				PrintMessage(Message::mkError, "Could not move file %s to %s: %s", szSrcFile, szDstFile,
					Util::GetLastErrorMessage(szErrBuf, sizeof(szErrBuf)));
				bOK = false;
			}
		}
	}

	if (bOK && !Util::DeleteDirectoryWithContent(m_szInterDir, szErrBuf, sizeof(szErrBuf)))
	{
		PrintMessage(Message::mkWarning, "Could not delete intermediate directory %s: %s", m_szInterDir, szErrBuf);
	}

	return bOK;
}

void MoveController::AddMessage(Message::EKind eKind, const char* szText)
{
	m_pPostInfo->GetNZBInfo()->AddMessage(eKind, szText);
}

void CleanupController::StartJob(PostInfo* pPostInfo)
{
	CleanupController* pCleanupController = new CleanupController();
	pCleanupController->m_pPostInfo = pPostInfo;
	pCleanupController->SetAutoDestroy(false);

	pPostInfo->SetPostThread(pCleanupController);

	pCleanupController->Start();
}

void CleanupController::Run()
{
	// the locking is needed for accessing the members of NZBInfo
	DownloadQueue::Lock();

	char szNZBName[1024];
	strncpy(szNZBName, m_pPostInfo->GetNZBInfo()->GetName(), 1024);
	szNZBName[1024-1] = '\0';

	char szInfoName[1024];
	snprintf(szInfoName, 1024, "cleanup for %s", m_pPostInfo->GetNZBInfo()->GetName());
	szInfoName[1024-1] = '\0';
	SetInfoName(szInfoName);

	strncpy(m_szDestDir, m_pPostInfo->GetNZBInfo()->GetDestDir(), 1024);
	m_szDestDir[1024-1] = '\0';

	bool bInterDir = strlen(g_pOptions->GetInterDir()) > 0 &&
		!strncmp(m_szDestDir, g_pOptions->GetInterDir(), strlen(g_pOptions->GetInterDir()));
	if (bInterDir)
	{
		m_pPostInfo->GetNZBInfo()->BuildFinalDirName(m_szFinalDir, 1024);
		m_szFinalDir[1024-1] = '\0';
	}
	else
	{
		m_szFinalDir[0] = '\0';
	}

	DownloadQueue::Unlock();

	PrintMessage(Message::mkInfo, "Cleaning up %s", szNZBName);

	bool bDeleted = false;
	bool bOK = Cleanup(m_szDestDir, &bDeleted);

	if (bOK && m_szFinalDir[0] != '\0')
	{
		bool bDeleted2 = false;
		bOK = Cleanup(m_szFinalDir, &bDeleted2);
		bDeleted = bDeleted || bDeleted2;
	}

	szInfoName[0] = 'C'; // uppercase

	if (bOK && bDeleted)
	{
		PrintMessage(Message::mkInfo, "%s successful", szInfoName);
		m_pPostInfo->GetNZBInfo()->SetCleanupStatus(NZBInfo::csSuccess);
	}
	else if (bOK)
	{
		PrintMessage(Message::mkInfo, "Nothing to cleanup for %s", szNZBName);
		m_pPostInfo->GetNZBInfo()->SetCleanupStatus(NZBInfo::csSuccess);
	}
	else
	{
		PrintMessage(Message::mkError, "%s failed", szInfoName);
		m_pPostInfo->GetNZBInfo()->SetCleanupStatus(NZBInfo::csFailure);
	}

	m_pPostInfo->SetStage(PostInfo::ptQueued);
	m_pPostInfo->SetWorking(false);
}

bool CleanupController::Cleanup(const char* szDestDir, bool *bDeleted)
{
	*bDeleted = false;
	bool bOK = true;

	DirBrowser dir(szDestDir);
	while (const char* filename = dir.Next())
	{
		char szFullFilename[1024];
		snprintf(szFullFilename, 1024, "%s%c%s", szDestDir, PATH_SEPARATOR, filename);
		szFullFilename[1024-1] = '\0';

		bool bIsDir = Util::DirectoryExists(szFullFilename);

		if (strcmp(filename, ".") && strcmp(filename, "..") && bIsDir)
		{
			bOK &= Cleanup(szFullFilename, bDeleted);
		}

		// check file extension
		bool bDeleteIt = Util::MatchFileExt(filename, g_pOptions->GetExtCleanupDisk(), ",;") && !bIsDir;

		if (bDeleteIt)
		{
			PrintMessage(Message::mkInfo, "Deleting file %s", filename);
			if (remove(szFullFilename) != 0)
			{
				char szErrBuf[256];
				PrintMessage(Message::mkError, "Could not delete file %s: %s", szFullFilename, Util::GetLastErrorMessage(szErrBuf, sizeof(szErrBuf)));
				bOK = false;
			}

			*bDeleted = true;
		}
	}

	return bOK;
}

void CleanupController::AddMessage(Message::EKind eKind, const char* szText)
{
	m_pPostInfo->GetNZBInfo()->AddMessage(eKind, szText);
}
