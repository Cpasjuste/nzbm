/*
 *  This file is part of nzbget
 *
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
 * $Revision: 1205 $
 * $Date: 2015-02-07 20:17:49 +0100 (sam. 07 févr. 2015) $
 *
 */


#ifndef QUEUEEDITOR_H
#define QUEUEEDITOR_H

#include <vector>

#include "DownloadInfo.h"

class QueueEditor
{
public:
	class EditItem
	{
	public:
		int			m_iOffset;
		FileInfo*	m_pFileInfo;
		NZBInfo*	m_pNZBInfo;

		EditItem(FileInfo* pFileInfo, NZBInfo* pNZBInfo, int iOffset);
	};

	typedef std::vector<EditItem*> ItemList;

private:
	DownloadQueue*			m_pDownloadQueue;

private:
	FileInfo*				FindFileInfo(int iID);
	bool					InternEditList(ItemList* pItemList, IDList* pIDList, DownloadQueue::EEditAction eAction, int iOffset, const char* szText);
	void					PrepareList(ItemList* pItemList, IDList* pIDList, DownloadQueue::EEditAction eAction, int iOffset);
	bool					BuildIDListFromNameList(IDList* pIDList, NameList* pNameList, DownloadQueue::EMatchMode eMatchMode, DownloadQueue::EEditAction eAction);
	bool					EditGroup(NZBInfo* pNZBInfo, DownloadQueue::EEditAction eAction, int iOffset, const char* szText);
	void					PauseParsInGroups(ItemList* pItemList, bool bExtraParsOnly);
	void					PausePars(FileList* pFileList, bool bExtraParsOnly);
	void					SetNZBPriority(NZBInfo* pNZBInfo, const char* szPriority);
	void					SetNZBCategory(NZBInfo* pNZBInfo, const char* szCategory, bool bApplyParams);
	void					SetNZBName(NZBInfo* pNZBInfo, const char* szName);
	bool					CanCleanupDisk(NZBInfo* pNZBInfo);
	bool					MergeGroups(ItemList* pItemList);
	bool					SortGroups(ItemList* pItemList, const char* szSort);
	bool					SplitGroup(ItemList* pItemList, const char* szName);
	bool					DeleteUrl(NZBInfo* pNZBInfo, DownloadQueue::EEditAction eAction);
	void					ReorderFiles(ItemList* pItemList);
	void					SetNZBParameter(NZBInfo* pNZBInfo, const char* szParamString);
	void					SetNZBDupeParam(NZBInfo* pNZBInfo, DownloadQueue::EEditAction eAction, const char* szText);
	void					PauseUnpauseEntry(FileInfo* pFileInfo, bool bPause);
	void					DeleteEntry(FileInfo* pFileInfo);
	void					MoveEntry(FileInfo* pFileInfo, int iOffset);
	void					MoveGroup(NZBInfo* pNZBInfo, int iOffset);

public:
							QueueEditor();                
							~QueueEditor();
	bool					EditEntry(DownloadQueue* pDownloadQueue, int ID, DownloadQueue::EEditAction eAction, int iOffset, const char* szText);
	bool					EditList(DownloadQueue* pDownloadQueue, IDList* pIDList, NameList* pNameList, DownloadQueue::EMatchMode eMatchMode, DownloadQueue::EEditAction eAction, int iOffset, const char* szText);
};

#endif
