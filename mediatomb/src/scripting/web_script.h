/*  scripting.h - this file is part of MediaTomb.
                                                                                
    Copyright (C) 2005 Gena Batyan <bgeradz@deadlock.dhs.org>,
                       Sergey Bostandzhyan <jin@deadlock.dhs.org>
                                                                                
    MediaTomb is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
                                                                                
    MediaTomb is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
                                                                                
    You should have received a copy of the GNU General Public License
    along with MediaTomb; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef __SCRIPTING_WEB_SCRIPT_H__
#define __SCRIPTING_WEB_SCRIPT_H__

#include "common.h"
#include "script.h"
#include "mxml/mxml.h"

class WebScript : public Script
{
public:
	WebScript(zmm::Ref<Runtime> runtime, zmm::String srcPath, JSObject *glob = NULL);
    zmm::String process(zmm::Ref<mxml::Element> root = nil);	
    zmm::String processUnlocked(zmm::Ref<mxml::Element> root = nil);	

    zmm::Ref<zmm::StringBuffer> output;
    zmm::Ref<zmm::Array<zmm::StringBase> > fragments;
    zmm::Ref<zmm::Array<WebScript> > includedScripts;
    int includeIndex;
    zmm::String sessionID;

    inline void setSessionID(zmm::String sessionID)
    { this->sessionID = sessionID; }

protected:
    long srcCompileTime;
    zmm::String srcPath;

    void xml2js(zmm::Ref<mxml::Element> el, JSObject *js);
    void checkCompile();
    void compileTemplate(zmm::String content);
            
    long getMtime();
    void printFrag(zmm::String str);
};

#endif // __SCRIPTING_WEB_SCRIPT_H__

