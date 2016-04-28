/*
 *  This file is part of nzbget. See <http://nzbget.net>.
 *
 *  Copyright (C) 2007-2016 Andrey Prygunkov <hugbug@users.sourceforge.net>
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
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#ifndef QUEUEEDITOR_H
#define QUEUEEDITOR_H

#include "DownloadInfo.h"

class QueueEditor
{
public:
	bool EditEntry(DownloadQueue* downloadQueue, int ID, DownloadQueue::EEditAction action, int offset, const char* text);
	bool EditList(DownloadQueue* downloadQueue, IdList* idList, NameList* nameList, DownloadQueue::EMatchMode matchMode, DownloadQueue::EEditAction action, int offset, const char* text);

private:
	class EditItem
	{
	public:
		int m_offset;
		FileInfo* m_fileInfo;
		NzbInfo* m_nzbInfo;

		EditItem(FileInfo* fileInfo, NzbInfo* nzbInfo, int offset) :
			m_fileInfo(fileInfo), m_nzbInfo(nzbInfo), m_offset(offset) {}
	};

	typedef std::vector<EditItem> ItemList;

	DownloadQueue* m_downloadQueue;

	FileInfo* FindFileInfo(int id);
	bool InternEditList(ItemList* itemList, IdList* idList, DownloadQueue::EEditAction action, int offset, const char* text);
	void PrepareList(ItemList* itemList, IdList* idList, DownloadQueue::EEditAction action, int offset);
	bool BuildIdListFromNameList(IdList* idList, NameList* nameList, DownloadQueue::EMatchMode matchMode, DownloadQueue::EEditAction action);
	bool EditGroup(NzbInfo* nzbInfo, DownloadQueue::EEditAction action, int offset, const char* text);
	void PauseParsInGroups(ItemList* itemList, bool extraParsOnly);
	void PausePars(RawFileList* fileList, bool extraParsOnly);
	void SetNzbPriority(NzbInfo* nzbInfo, const char* priority);
	void SetNzbCategory(NzbInfo* nzbInfo, const char* category, bool applyParams);
	void SetNzbName(NzbInfo* nzbInfo, const char* name);
	bool CanCleanupDisk(NzbInfo* nzbInfo);
	bool MergeGroups(ItemList* itemList);
	bool SortGroups(ItemList* itemList, const char* sort);
	bool SplitGroup(ItemList* itemList, const char* name);
	bool DeleteUrl(NzbInfo* nzbInfo, DownloadQueue::EEditAction action);
	void ReorderFiles(ItemList* itemList);
	void SetNzbParameter(NzbInfo* nzbInfo, const char* paramString);
	void SetNzbDupeParam(NzbInfo* nzbInfo, DownloadQueue::EEditAction action, const char* text);
	void PauseUnpauseEntry(FileInfo* fileInfo, bool pause);
	void DeleteEntry(FileInfo* fileInfo);
	void MoveEntry(FileInfo* fileInfo, int offset);
	void MoveGroup(NzbInfo* nzbInfo, int offset);

	friend class GroupSorter;
};

#endif
