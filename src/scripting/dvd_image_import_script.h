/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    dvd_image_import_script.h - this file is part of MediaTomb.
    
    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>
    
    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>
    
    MediaTomb is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.
    
    MediaTomb is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    version 2 along with MediaTomb; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
    
    $Id$
*/

/// \file dvd_image_import_script.h

#ifndef __SCRIPTING_DVD_IMAGE_IMPORT_SCRIPT_H__
#define __SCRIPTING_DVD_IMAGE_IMPORT_SCRIPT_H__

#include "common.h"
#include "script.h"
#include "cds_objects.h"
#include "content_manager.h"

class DVDImportScript : public Script
{
public:
    DVDImportScript(zmm::Ref<Runtime> runtime);
    ~DVDImportScript();
    /// \brief Adds a DVD object to the database
    /// 
    /// \param title DVD title number
    /// \param chapter DVD chapter 
    /// \param audio track DVD audio track
    void addDVDObject(zmm::Ref<CdsObject> obj, int title, int chapter, 
                      int audio_track, zmm::String chain, 
                      zmm::String containerclass);
    void processDVDObject(zmm::Ref<CdsObject> obj);
    virtual script_class_t whoami() { return S_DVD; }

private:
    int currentObjectID;
    zmm::Ref<CMTask> currentTask;
    JSObject *root;
    zmm::String mimetype;

};

#endif // __SCRIPTING_DVD_IMAGE_IMPORT_SCRIPT_H__
