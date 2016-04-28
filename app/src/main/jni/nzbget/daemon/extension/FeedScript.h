/*
 *  This file is part of nzbget. See <http://nzbget.net>.
 *
 *  Copyright (C) 2015-2016 Andrey Prygunkov <hugbug@users.sourceforge.net>
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


#ifndef FEEDSCRIPT_H
#define FEEDSCRIPT_H

#include "NzbScript.h"

class FeedScriptController : public NzbScriptController
{
public:
	static void ExecuteScripts(const char* feedScript, const char* feedFile, int feedId);

protected:
	virtual void ExecuteScript(ScriptConfig::Script* script);

private:
	const char* m_feedFile;
	int m_feedId;

	void PrepareParams(const char* scriptName);
};

#endif
