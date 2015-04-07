/*
 *  This file is part of nzbget
 *
 *  Copyright (C) 2014 Andrey Prygunkov <hugbug@users.sourceforge.net>
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
 * $Revision: 1238 $
 * $Date: 2015-03-21 17:12:01 +0100 (sam. 21 mars 2015) $
 *
 */


#ifndef STATMETER_H
#define STATMETER_H

#include <vector>
#include <time.h>

#include "Log.h"
#include "Thread.h"

class ServerVolume
{
public:
	typedef std::vector<long long>	VolumeArray;

private:
	VolumeArray			m_BytesPerSeconds;
	VolumeArray			m_BytesPerMinutes;
	VolumeArray			m_BytesPerHours;
	VolumeArray			m_BytesPerDays;
	int					m_iFirstDay;
	long long			m_lTotalBytes;
	long long			m_lCustomBytes;
	time_t				m_tDataTime;
	time_t				m_tCustomTime;
	int					m_iSecSlot;
	int					m_iMinSlot;
	int					m_iHourSlot;
	int					m_iDaySlot;

public:
						ServerVolume();
	VolumeArray*		BytesPerSeconds() { return &m_BytesPerSeconds; }
	VolumeArray*		BytesPerMinutes() { return &m_BytesPerMinutes; }
	VolumeArray*		BytesPerHours() { return &m_BytesPerHours; }
	VolumeArray*		BytesPerDays() { return &m_BytesPerDays; }
	void				SetFirstDay(int iFirstDay) { m_iFirstDay = iFirstDay; }
	int					GetFirstDay() { return m_iFirstDay; }
	void				SetTotalBytes(long long lTotalBytes) { m_lTotalBytes = lTotalBytes; }
	long long			GetTotalBytes() { return m_lTotalBytes; }
	void				SetCustomBytes(long long lCustomBytes) { m_lCustomBytes = lCustomBytes; }
	long long			GetCustomBytes() { return m_lCustomBytes; }
	int					GetSecSlot() { return m_iSecSlot; }
	int					GetMinSlot() { return m_iMinSlot; }
	int					GetHourSlot() { return m_iHourSlot; }
	int					GetDaySlot() { return m_iDaySlot; }
	time_t				GetDataTime() { return m_tDataTime; }
	void				SetDataTime(time_t tDataTime) { m_tDataTime = tDataTime; }
	time_t				GetCustomTime() { return m_tCustomTime; }
	void				SetCustomTime(time_t tCustomTime) { m_tCustomTime = tCustomTime; }

	void				AddData(int iBytes);
	void				CalcSlots(time_t tLocCurTime);
	void				ResetCustom();
	void				LogDebugInfo();
};

typedef std::vector<ServerVolume*>	ServerVolumes;

class StatMeter : public Debuggable
{
private:
	// speed meter
	static const int	SPEEDMETER_SLOTS = 30;	  
	static const int	SPEEDMETER_SLOTSIZE = 1;  //Split elapsed time into this number of secs.
	int					m_iSpeedBytes[SPEEDMETER_SLOTS];
	long long			m_iSpeedTotalBytes;
	int					m_iSpeedTime[SPEEDMETER_SLOTS];
	int					m_iSpeedStartTime; 
	time_t				m_tSpeedCorrection;
	int					m_iSpeedBytesIndex;
	int					m_iCurSecBytes;
	time_t				m_tCurSecTime;
#ifdef HAVE_SPINLOCK
	SpinLock			m_spinlockSpeed;
#else
	Mutex				m_mutexSpeed;
#endif

	// time
	long long			m_iAllBytes;
	time_t				m_tStartServer;
	time_t				m_tLastCheck;
	time_t				m_tLastTimeOffset;
	time_t				m_tStartDownload;
	time_t				m_tPausedFrom;
	bool				m_bStandBy;
	Mutex				m_mutexStat;

	// data volume
	bool				m_bStatChanged;
	ServerVolumes		m_ServerVolumes;
	Mutex				m_mutexVolume;

	void				ResetSpeedStat();
	void				AdjustTimeOffset();

protected:
	virtual void		LogDebugInfo();

public:
						StatMeter();
						~StatMeter();
	void				Init();
	int					CalcCurrentDownloadSpeed();
	int					CalcMomentaryDownloadSpeed();
	void				AddSpeedReading(int iBytes);
	void				AddServerData(int iBytes, int iServerID);
	void				CalcTotalStat(int* iUpTimeSec, int* iDnTimeSec, long long* iAllBytes, bool* bStandBy);
	bool				GetStandBy() { return m_bStandBy; }
	void				IntervalCheck();
	void				EnterLeaveStandBy(bool bEnter);
	ServerVolumes*		LockServerVolumes();
	void				UnlockServerVolumes();
	void				Save();
	bool				Load(bool* pPerfectServerMatch);
};

#endif
