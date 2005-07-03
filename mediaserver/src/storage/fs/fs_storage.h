/*  fs_storage.h - this file is part of MediaTomb.

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

#ifndef __FS_STORAGE_H__
#define __FS_STORAGE_H__

#include "common.h"
#include "storage.h"
#include "dictionary.h"
#include "rexp.h"

class FsStorage : public Storage
{
public:
    FsStorage();
    virtual ~FsStorage();

    virtual void init();
    virtual void addObject(zmm::Ref<CdsObject> object);
    virtual void updateObject(zmm::Ref<CdsObject> object);
    virtual void eraseObject(zmm::Ref<CdsObject> object);

    virtual zmm::Ref<zmm::Array<CdsObject> > browse(zmm::Ref<BrowseParam> param);
    virtual zmm::Ref<zmm::Array<zmm::StringBase> > getMimeTypes();
    virtual zmm::Ref<CdsObject> findObjectByTitle(zmm::String title, zmm::String parentID);

    bool fileAllowed(zmm::String path);
protected:
    zmm::Ref<zmm::Array<RExp> > include_rules;
};

#endif // __FS_STORAGE_H__

