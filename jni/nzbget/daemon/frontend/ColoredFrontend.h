/*
 *  This file if part of nzbget
 *
 *  Copyright (C) 2004  Sven Henkel <sidddy@users.sourceforge.net>
 *  Copyright (C) 2007  Andrey Prygunkov <hugbug@users.sourceforge.net>
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
 * $Revision: 956 $
 * $Date: 2014-02-24 23:11:14 +0100 (lun. 24 févr. 2014) $
 *
 */


#ifndef COLOREDFRONTEND_H
#define COLOREDFRONTEND_H

#include "LoggableFrontend.h"
#include "Log.h"

class ColoredFrontend : public LoggableFrontend
{
private:
	bool			m_bNeedGoBack;

#ifdef WIN32
	HANDLE			m_hConsole;
#endif

protected:
	virtual void 	BeforePrint();
	virtual void	PrintMessage(Message* pMessage);
	virtual void 	PrintStatus();
	virtual void 	PrintSkip();
	virtual void	BeforeExit();

public:
	ColoredFrontend();
};

#endif
