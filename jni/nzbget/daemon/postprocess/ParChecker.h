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
 * $Revision: 1139 $
 * $Date: 2014-10-09 23:11:42 +0200 (Thu, 09 Oct 2014) $
 *
 */


#ifndef PARCHECKER_H
#define PARCHECKER_H

#ifndef DISABLE_PARCHECK

#include <deque>
#include <vector>
#include <string>

#include "Thread.h"
#include "Log.h"

class ParChecker : public Thread
{
public:
	enum EStatus
	{
		psFailed,
		psRepairPossible,
		psRepaired,
		psRepairNotNeeded
	};

	enum EStage
	{
		ptLoadingPars,
		ptVerifyingSources,
		ptRepairing,
		ptVerifyingRepaired,
	};

	enum EFileStatus
	{
		fsUnknown,
		fsSuccess,
		fsPartial,
		fsFailure
	};

	class Segment
	{
	private:
		bool				m_bSuccess;
		long long			m_iOffset;
		int					m_iSize;
		unsigned long		m_lCrc;

	public:
							Segment(bool bSuccess, long long iOffset, int iSize, unsigned long lCrc);
		bool 				GetSuccess() { return m_bSuccess; }
		long long			GetOffset() { return m_iOffset; }
		int 				GetSize() { return m_iSize; }
		unsigned long		GetCrc() { return m_lCrc; }
	};

	typedef std::deque<Segment*>	SegmentListBase;

	class SegmentList : public SegmentListBase
	{
	public:
							~SegmentList();
	};

	typedef std::deque<char*>		FileList;
	typedef std::deque<void*>		SourceList;
	typedef std::vector<bool>		ValidBlocks;

	friend class Repairer;
	
private:
	char*				m_szInfoName;
	char*				m_szDestDir;
	char*				m_szNZBName;
	const char*			m_szParFilename;
	EStatus				m_eStatus;
	EStage				m_eStage;
	// declared as void* to prevent the including of libpar2-headers into this header-file
	void*				m_pRepairer;
	char*				m_szErrMsg;
	FileList			m_QueuedParFiles;
	Mutex			 	m_mutexQueuedParFiles;
	bool				m_bQueuedParFilesChanged;
	FileList			m_ProcessedFiles;
	int					m_iProcessedFiles;
	int					m_iFilesToRepair;
	int					m_iExtraFiles;
	bool				m_bVerifyingExtraFiles;
	char*				m_szProgressLabel;
	int					m_iFileProgress;
	int					m_iStageProgress;
	bool				m_bCancelled;
	SourceList			m_sourceFiles;
	std::string			m_lastFilename;
	bool				m_bHasDamagedFiles;
	bool				m_bParQuick;
	bool				m_bForceRepair;

	void				Cleanup();
	EStatus				RunParCheckAll();
	EStatus				RunParCheck(const char* szParFilename);
	int					PreProcessPar();
	bool				LoadMainParBak();
	int					ProcessMorePars();
	bool				LoadMorePars();
	bool				AddSplittedFragments();
	bool				AddMissingFiles();
	bool				IsProcessedFile(const char* szFilename);
	void				WriteBrokenLog(EStatus eStatus);
	void				SaveSourceList();
	void				DeleteLeftovers();
	void				signal_filename(std::string str);
	void				signal_progress(double progress);
	void				signal_done(std::string str, int available, int total);
	// declared as void* to prevent the including of libpar2-headers into this header-file
	// DiskFile* pDiskfile, Par2RepairerSourceFile* pSourcefile
	EFileStatus			VerifyDataFile(void* pDiskfile, void* pSourcefile, int* pAvailableBlocks);
	bool				VerifySuccessDataFile(void* pDiskfile, void* pSourcefile, unsigned long lDownloadCrc);
	bool				VerifyPartialDataFile(void* pDiskfile, void* pSourcefile, SegmentList* pSegments, ValidBlocks* pValidBlocks);
	bool				SmartCalcFileRangeCrc(FILE* pFile, long long lStart, long long lEnd, SegmentList* pSegments,
							unsigned long* pDownloadCrc);
	bool				DumbCalcFileRangeCrc(FILE* pFile, long long lStart, long long lEnd, unsigned long* pDownloadCrc);

protected:
	/**
	* Unpause par2-files
	* returns true, if the files with required number of blocks were unpaused,
	* or false if there are no more files in queue for this collection or not enough blocks
	*/
	virtual bool		RequestMorePars(int iBlockNeeded, int* pBlockFound) = 0;
	virtual void		UpdateProgress() {}
	virtual void		Completed() {}
	virtual void		PrintMessage(Message::EKind eKind, const char* szFormat, ...) {}
	virtual void		RegisterParredFile(const char* szFilename) {}
	virtual bool		IsParredFile(const char* szFilename) { return false; }
	virtual EFileStatus	FindFileCrc(const char* szFilename, unsigned long* lCrc, SegmentList* pSegments) { return fsUnknown; }
	EStage				GetStage() { return m_eStage; }
	const char*			GetProgressLabel() { return m_szProgressLabel; }
	int					GetFileProgress() { return m_iFileProgress; }
	int					GetStageProgress() { return m_iStageProgress; }

public:
						ParChecker();
	virtual				~ParChecker();
	virtual void		Run();
	void				SetDestDir(const char* szDestDir);
	const char*			GetParFilename() { return m_szParFilename; }
	const char*			GetInfoName() { return m_szInfoName; }
	void				SetInfoName(const char* szInfoName);
	void				SetNZBName(const char* szNZBName);
	void				SetParQuick(bool bParQuick) { m_bParQuick = bParQuick; }
	bool				GetParQuick() { return m_bParQuick; }
	void				SetForceRepair(bool bForceRepair) { m_bForceRepair = bForceRepair; }
	bool				GetForceRepair() { return m_bForceRepair; }
	EStatus				GetStatus() { return m_eStatus; }
	void				AddParFile(const char* szParFilename);
	void				QueueChanged();
	void				Cancel();
	bool				GetCancelled() { return m_bCancelled; }
};

#endif

#endif
