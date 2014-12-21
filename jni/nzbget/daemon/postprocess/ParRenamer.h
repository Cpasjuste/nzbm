/*
 *  This file is part of nzbget
 *
 *  Copyright (C) 2013-2014 Andrey Prygunkov <hugbug@users.sourceforge.net>
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
 * $Revision: 1070 $
 * $Date: 2014-07-27 23:59:00 +0200 (Sun, 27 Jul 2014) $
 *
 */


#ifndef PARRENAMER_H
#define PARRENAMER_H

#ifndef DISABLE_PARCHECK

#include <deque>

#include "Thread.h"
#include "Log.h"

class ParRenamer : public Thread
{
public:
	enum EStatus
	{
		psFailed,
		psSuccess
	};
	
	class FileHash
	{
	private:
		char*			m_szFilename;
		char*			m_szHash;
		bool			m_bFileExists;

	public:
						FileHash(const char* szFilename, const char* szHash);
						~FileHash();
		const char*		GetFilename() { return m_szFilename; }
		const char*		GetHash() { return m_szHash; }
		bool			GetFileExists() { return m_bFileExists; }
		void			SetFileExists(bool bFileExists) { m_bFileExists = bFileExists; }
	};

	typedef std::deque<FileHash*>		FileHashList;
	typedef std::deque<char*>			DirList;
	
private:
	char*				m_szInfoName;
	char*				m_szDestDir;
	EStatus				m_eStatus;
	char*				m_szProgressLabel;
	int					m_iStageProgress;
	bool				m_bCancelled;
	DirList				m_DirList;
	FileHashList		m_FileHashList;
	int					m_iFileCount;
	int					m_iCurFile;
	int					m_iRenamedCount;
	bool				m_bHasMissedFiles;
	bool				m_bDetectMissing;

	void				Cleanup();
	void				ClearHashList();
	void				BuildDirList(const char* szDestDir);
	void				CheckDir(const char* szDestDir);
	void				LoadParFiles(const char* szDestDir);
	void				LoadParFile(const char* szParFilename);
	void				CheckFiles(const char* szDestDir, bool bRenamePars);
	void				CheckRegularFile(const char* szDestDir, const char* szFilename);
	void				CheckParFile(const char* szDestDir, const char* szFilename);
	bool				IsSplittedFragment(const char* szFilename, const char* szCorrectName);
	void				CheckMissing();
	void				RenameFile(const char* szSrcFilename, const char* szDestFileName);

protected:
	virtual void		UpdateProgress() {}
	virtual void		Completed() {}
	virtual void		PrintMessage(Message::EKind eKind, const char* szFormat, ...) {}
	virtual void		RegisterParredFile(const char* szFilename) {}
	virtual void		RegisterRenamedFile(const char* szOldFilename, const char* szNewFileName) {}
	const char*			GetProgressLabel() { return m_szProgressLabel; }
	int					GetStageProgress() { return m_iStageProgress; }

public:
						ParRenamer();
	virtual				~ParRenamer();
	virtual void		Run();
	void				SetDestDir(const char* szDestDir);
	const char*			GetInfoName() { return m_szInfoName; }
	void				SetInfoName(const char* szInfoName);
	void				SetStatus(EStatus eStatus);
	EStatus				GetStatus() { return m_eStatus; }
	void				Cancel();
	bool				GetCancelled() { return m_bCancelled; }
	bool				HasMissedFiles() { return m_bHasMissedFiles; }
	void				SetDetectMissing(bool bDetectMissing) { m_bDetectMissing = bDetectMissing; }
};

#endif

#endif
