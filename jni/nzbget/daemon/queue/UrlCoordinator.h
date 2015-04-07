/*
 *  This file is part of nzbget
 *
 *  Copyright (C) 2012-2014 Andrey Prygunkov <hugbug@users.sourceforge.net>
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
 * $Revision: 967 $
 * $Date: 2014-03-20 22:14:39 +0100 (jeu. 20 mars 2014) $
 *
 */


#ifndef URLCOORDINATOR_H
#define URLCOORDINATOR_H

#include <deque>
#include <list>
#include <time.h>

#include "Log.h"
#include "Thread.h"
#include "WebDownloader.h"
#include "DownloadInfo.h"
#include "Observer.h"

class UrlDownloader;

class UrlCoordinator : public Thread, public Observer, public Debuggable
{
private:
	typedef std::list<UrlDownloader*>	ActiveDownloads;

private:
	ActiveDownloads			m_ActiveDownloads;
	bool					m_bHasMoreJobs;
	bool					m_bForce;

	NZBInfo*				GetNextUrl(DownloadQueue* pDownloadQueue);
	void					StartUrlDownload(NZBInfo* pNZBInfo);
	void					UrlCompleted(UrlDownloader* pUrlDownloader);
	void					ResetHangingDownloads();

protected:
	virtual void			LogDebugInfo();

public:
							UrlCoordinator();                
	virtual					~UrlCoordinator();
	virtual void			Run();
	virtual void 			Stop();
	void					Update(Subject* pCaller, void* pAspect);

	// Editing the queue
	void					AddUrlToQueue(NZBInfo* pNZBInfo, bool bAddTop);
	bool					HasMoreJobs() { return m_bHasMoreJobs; }
	bool					DeleteQueueEntry(DownloadQueue* pDownloadQueue, NZBInfo* pNZBInfo, bool bAvoidHistory);
};

class UrlDownloader : public WebDownloader
{
private:
	NZBInfo*				m_pNZBInfo;
	char*					m_szCategory;

protected:
	virtual void			ProcessHeader(const char* szLine);

public:
							UrlDownloader();
							~UrlDownloader();
	void					SetNZBInfo(NZBInfo* pNZBInfo) { m_pNZBInfo = pNZBInfo; }
	NZBInfo*				GetNZBInfo() { return m_pNZBInfo; }
	const char*				GetCategory() { return m_szCategory; }
};

#endif
