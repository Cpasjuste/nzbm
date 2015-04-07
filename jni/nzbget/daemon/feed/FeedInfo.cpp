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
 * $Revision: 0 $
 * $Date: 2014-09-27 23:04:06 +0200 (sam. 27 sept. 2014) $
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
#include <sys/stat.h>

#include "nzbget.h"
#include "FeedInfo.h"
#include "DupeCoordinator.h"
#include "Util.h"

extern DupeCoordinator* g_pDupeCoordinator;

FeedInfo::FeedInfo(int iID, const char* szName, const char* szUrl, int iInterval,
	const char* szFilter, bool bPauseNzb, const char* szCategory, int iPriority)
{
	m_iID = iID;
	m_szName = strdup(szName ? szName : "");
	m_szUrl = strdup(szUrl ? szUrl : "");
	m_szFilter = strdup(szFilter ? szFilter : "");
	m_iFilterHash = Util::HashBJ96(m_szFilter, strlen(m_szFilter), 0);
	m_szCategory = strdup(szCategory ? szCategory : "");
	m_iInterval = iInterval;
	m_bPauseNzb = bPauseNzb;
	m_iPriority = iPriority;
	m_tLastUpdate = 0;
	m_bPreview = false;
	m_eStatus = fsUndefined;
	m_szOutputFilename = NULL;
	m_bFetch = false;
	m_bForce = false;
}

FeedInfo::~FeedInfo()
{
	free(m_szName);
	free(m_szUrl);
	free(m_szFilter);
	free(m_szCategory);
	free(m_szOutputFilename);
}

void FeedInfo::SetOutputFilename(const char* szOutputFilename)
{
	free(m_szOutputFilename);
	m_szOutputFilename = strdup(szOutputFilename);
}


FeedItemInfo::Attr::Attr(const char* szName, const char* szValue)
{
	m_szName = strdup(szName ? szName : "");
	m_szValue = strdup(szValue ? szValue : "");
}

FeedItemInfo::Attr::~Attr()
{
	free(m_szName);
	free(m_szValue);
}


FeedItemInfo::Attributes::~Attributes()
{
	for (iterator it = begin(); it != end(); it++)
	{
		delete *it;
	}
}

void FeedItemInfo::Attributes::Add(const char* szName, const char* szValue)
{
	push_back(new Attr(szName, szValue));
}

FeedItemInfo::Attr* FeedItemInfo::Attributes::Find(const char* szName)
{
	for (iterator it = begin(); it != end(); it++)
	{
		Attr* pAttr = *it;
		if (!strcasecmp(pAttr->GetName(), szName))
		{
			return pAttr;
		}
	}

	return NULL;
}


FeedItemInfo::FeedItemInfo()
{
	m_pSharedFeedData = NULL;
	m_szTitle = NULL;
	m_szFilename = NULL;
	m_szUrl = NULL;
	m_szCategory = strdup("");
	m_lSize = 0;
	m_tTime = 0;
	m_iImdbId = 0;
	m_iRageId = 0;
	m_szDescription = strdup("");
	m_szSeason = NULL;
	m_szEpisode = NULL;
	m_iSeasonNum = 0;
	m_iEpisodeNum = 0;
	m_bSeasonEpisodeParsed = false;
	m_szAddCategory = strdup("");
	m_bPauseNzb = false;
	m_iPriority = 0;
	m_eStatus = isUnknown;
	m_eMatchStatus = msIgnored;
	m_iMatchRule = 0;
	m_szDupeKey = NULL;
	m_iDupeScore = 0;
	m_eDupeMode = dmScore;
	m_szDupeStatus = NULL;
}

FeedItemInfo::~FeedItemInfo()
{
	free(m_szTitle);
	free(m_szFilename);
	free(m_szUrl);
	free(m_szCategory);
	free(m_szDescription);
	free(m_szSeason);
	free(m_szEpisode);
	free(m_szAddCategory);
	free(m_szDupeKey);
	free(m_szDupeStatus);
}

void FeedItemInfo::SetTitle(const char* szTitle)
{
	free(m_szTitle);
	m_szTitle = szTitle ? strdup(szTitle) : NULL;
}

void FeedItemInfo::SetFilename(const char* szFilename)
{
	free(m_szFilename);
	m_szFilename = szFilename ? strdup(szFilename) : NULL;
}

void FeedItemInfo::SetUrl(const char* szUrl)
{
	free(m_szUrl);
	m_szUrl = szUrl ? strdup(szUrl) : NULL;
}

void FeedItemInfo::SetCategory(const char* szCategory)
{
	free(m_szCategory);
	m_szCategory = strdup(szCategory ? szCategory: "");
}

void FeedItemInfo::SetDescription(const char* szDescription)
{
	free(m_szDescription);
	m_szDescription = strdup(szDescription ? szDescription: "");
}

void FeedItemInfo::SetSeason(const char* szSeason)
{
	free(m_szSeason);
	m_szSeason = szSeason ? strdup(szSeason) : NULL;
	m_iSeasonNum = szSeason ? ParsePrefixedInt(szSeason) : 0;
}

void FeedItemInfo::SetEpisode(const char* szEpisode)
{
	free(m_szEpisode);
	m_szEpisode = szEpisode ? strdup(szEpisode) : NULL;
	m_iEpisodeNum = szEpisode ? ParsePrefixedInt(szEpisode) : 0;
}

int FeedItemInfo::ParsePrefixedInt(const char *szValue)
{
	const char* szVal = szValue;
	if (!strchr("0123456789", *szVal))
	{
		szVal++;
	}
	return atoi(szVal);
}

void FeedItemInfo::SetAddCategory(const char* szAddCategory)
{
	free(m_szAddCategory);
	m_szAddCategory = strdup(szAddCategory ? szAddCategory : "");
}

void FeedItemInfo::SetDupeKey(const char* szDupeKey)
{
	free(m_szDupeKey);
	m_szDupeKey = strdup(szDupeKey ? szDupeKey : "");
}

void FeedItemInfo::AppendDupeKey(const char* szExtraDupeKey)
{
	if (!m_szDupeKey || *m_szDupeKey == '\0' || !szExtraDupeKey || *szExtraDupeKey == '\0')
	{
		return;
	}

	int iLen = (m_szDupeKey ? strlen(m_szDupeKey) : 0) + 1 + strlen(szExtraDupeKey) + 1;
	char* szNewKey = (char*)malloc(iLen);
	snprintf(szNewKey, iLen, "%s-%s", m_szDupeKey, szExtraDupeKey);
	szNewKey[iLen - 1] = '\0';

	free(m_szDupeKey);
	m_szDupeKey = szNewKey;
}

void FeedItemInfo::BuildDupeKey(const char* szRageId, const char* szSeries)
{
	int iRageId = szRageId && *szRageId ? atoi(szRageId) : m_iRageId;

	free(m_szDupeKey);

	if (m_iImdbId != 0)
	{
		m_szDupeKey = (char*)malloc(20);
		snprintf(m_szDupeKey, 20, "imdb=%i", m_iImdbId);
	}
	else if (szSeries && *szSeries && GetSeasonNum() != 0 && GetEpisodeNum() != 0)
	{
		int iLen = strlen(szSeries) + 50;
		m_szDupeKey = (char*)malloc(iLen);
		snprintf(m_szDupeKey, iLen, "series=%s-%s-%s", szSeries, m_szSeason, m_szEpisode);
		m_szDupeKey[iLen-1] = '\0';
	}
	else if (iRageId != 0 && GetSeasonNum() != 0 && GetEpisodeNum() != 0)
	{
		m_szDupeKey = (char*)malloc(100);
		snprintf(m_szDupeKey, 100, "rageid=%i-%s-%s", iRageId, m_szSeason, m_szEpisode);
		m_szDupeKey[100-1] = '\0';
	}
	else
	{
		m_szDupeKey = strdup("");
	}
}

int FeedItemInfo::GetSeasonNum()
{
	if (!m_szSeason && !m_bSeasonEpisodeParsed)
	{
		ParseSeasonEpisode();
	}

	return m_iSeasonNum;
}

int FeedItemInfo::GetEpisodeNum()
{
	if (!m_szEpisode && !m_bSeasonEpisodeParsed)
	{
		ParseSeasonEpisode();
	}

	return m_iEpisodeNum;
}

void FeedItemInfo::ParseSeasonEpisode()
{
	m_bSeasonEpisodeParsed = true;

	RegEx* pRegEx = m_pSharedFeedData->GetSeasonEpisodeRegEx();

	if (pRegEx->Match(m_szTitle))
	{
		char szRegValue[100];
		char szValue[100];

		snprintf(szValue, 100, "S%02d", atoi(m_szTitle + pRegEx->GetMatchStart(1)));
		szValue[100-1] = '\0';
		SetSeason(szValue);

		int iLen = pRegEx->GetMatchLen(2);
		iLen = iLen < 99 ? iLen : 99;
		strncpy(szRegValue, m_szTitle + pRegEx->GetMatchStart(2), pRegEx->GetMatchLen(2));
		szRegValue[iLen] = '\0';
		snprintf(szValue, 100, "E%s", szRegValue);
		szValue[100-1] = '\0';
		Util::ReduceStr(szValue, "-", "");
		for (char* p = szValue; *p; p++) *p = toupper(*p); // convert string to uppercase e02 -> E02
		SetEpisode(szValue);
	}
}

const char* FeedItemInfo::GetDupeStatus()
{
	if (!m_szDupeStatus)
	{
		const char* szDupeStatusName[] = { "", "QUEUED", "DOWNLOADING", "3", "SUCCESS", "5", "6", "7", "WARNING",
			"9", "10", "11", "12", "13", "14", "15", "FAILURE" };
		char szStatuses[200];
		szStatuses[0] = '\0';

		DownloadQueue* pDownloadQueue = DownloadQueue::Lock();
		DupeCoordinator::EDupeStatus eDupeStatus = g_pDupeCoordinator->GetDupeStatus(pDownloadQueue, m_szTitle, m_szDupeKey);
		DownloadQueue::Unlock();

		for (int i = 1; i <= (int)DupeCoordinator::dsFailure; i = i << 1)
		{
			if (eDupeStatus & i)
			{
				if (*szStatuses)
				{
					strcat(szStatuses, ",");
				}
				strcat(szStatuses, szDupeStatusName[i]);
			}
		}

		m_szDupeStatus = strdup(szStatuses);
	}

	return m_szDupeStatus;
}


FeedHistoryInfo::FeedHistoryInfo(const char* szUrl, FeedHistoryInfo::EStatus eStatus, time_t tLastSeen)
{
	m_szUrl = szUrl ? strdup(szUrl) : NULL;
	m_eStatus = eStatus;
	m_tLastSeen = tLastSeen;
}

FeedHistoryInfo::~FeedHistoryInfo()
{
	free(m_szUrl);
}


FeedHistory::~FeedHistory()
{
	Clear();
}

void FeedHistory::Clear()
{
	for (iterator it = begin(); it != end(); it++)
	{
		delete *it;
	}
	clear();
}

void FeedHistory::Add(const char* szUrl, FeedHistoryInfo::EStatus eStatus, time_t tLastSeen)
{
	push_back(new FeedHistoryInfo(szUrl, eStatus, tLastSeen));
}

void FeedHistory::Remove(const char* szUrl)
{
	for (iterator it = begin(); it != end(); it++)
	{
		FeedHistoryInfo* pFeedHistoryInfo = *it;
		if (!strcmp(pFeedHistoryInfo->GetUrl(), szUrl))
		{
			delete pFeedHistoryInfo;
			erase(it);
			break;
		}
	}
}

FeedHistoryInfo* FeedHistory::Find(const char* szUrl)
{
	for (iterator it = begin(); it != end(); it++)
	{
		FeedHistoryInfo* pFeedHistoryInfo = *it;
		if (!strcmp(pFeedHistoryInfo->GetUrl(), szUrl))
		{
			return pFeedHistoryInfo;
		}
	}

	return NULL;
}


FeedItemInfos::FeedItemInfos()
{
	debug("Creating FeedItemInfos");
	
	m_iRefCount = 0;
}

FeedItemInfos::~FeedItemInfos()
{
	debug("Destroing FeedItemInfos");
	
	for (iterator it = begin(); it != end(); it++)
    {
        delete *it;
    }
}

void FeedItemInfos::Retain()
{
	m_iRefCount++;
}

void FeedItemInfos::Release()
{
	m_iRefCount--;
	if (m_iRefCount <= 0)
	{
		delete this;
	}
}

void FeedItemInfos::Add(FeedItemInfo* pFeedItemInfo)
{
	push_back(pFeedItemInfo);
	pFeedItemInfo->SetSharedFeedData(&m_SharedFeedData);
}


SharedFeedData::SharedFeedData()
{
	m_pSeasonEpisodeRegEx = NULL;
}

SharedFeedData::~SharedFeedData()
{
	delete m_pSeasonEpisodeRegEx;
}

RegEx* SharedFeedData::GetSeasonEpisodeRegEx()
{
	if (!m_pSeasonEpisodeRegEx)
	{
		m_pSeasonEpisodeRegEx = new RegEx("[^[:alnum:]]s?([0-9]+)[ex]([0-9]+(-?e[0-9]+)?)[^[:alnum:]]", 10);
	}

	return m_pSeasonEpisodeRegEx;
}
