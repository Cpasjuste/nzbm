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
 * $Revision: 1189 $
 * $Date: 2015-01-22 21:57:39 +0100 (jeu. 22 janv. 2015) $
 *
 */


#ifndef THREAD_H
#define THREAD_H

class Mutex
{
private:
	void*					m_pMutexObj;
	
public:
							Mutex();
							~Mutex();
	void					Lock();
	void					Unlock();
};

#ifdef HAVE_SPINLOCK
class SpinLock
{
private:
#ifdef WIN32
	void*					m_pSpinLockObj;
#else
	volatile void*			m_pSpinLockObj;
#endif
	
public:
							SpinLock();
							~SpinLock();
	void					Lock();
	void					Unlock();
};
#endif

class Thread
{
private:
	static Mutex*			m_pMutexThread;
	static int				m_iThreadCount;
	void*	 				m_pThreadObj;
	bool 					m_bRunning;
	bool					m_bStopped;
	bool					m_bAutoDestroy;

#ifdef WIN32
	static void __cdecl 	thread_handler(void* pObject);
#else
	static void				*thread_handler(void* pObject);
#endif

public:
							Thread();
	virtual 				~Thread();
	static void				Init();
	static void				Final();

	virtual void 			Start();
	virtual void 			Stop();
	virtual void 			Resume();
	bool					Kill();

	bool 					IsStopped() { return m_bStopped; };
	bool 					IsRunning()	const { return m_bRunning; }
	void 					SetRunning(bool bOnOff) { m_bRunning = bOnOff; }
	bool					GetAutoDestroy() { return m_bAutoDestroy; }
	void					SetAutoDestroy(bool bAutoDestroy) { m_bAutoDestroy = bAutoDestroy; }
	static int				GetThreadCount();

protected:
	virtual void 			Run() {}; // Virtual function - override in derivatives
};

#endif
