/*  sqlite3_storage.h - this file is part of MediaTomb.
                                                                                
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

#ifndef __SQLITE3_STORAGE_H__
#define __SQLITE3_STORAGE_H__

#include "storage.h"
#include "dictionary.h"

#include <sqlite3.h>
#include <pthread.h>

#define MAX_DELETE_QUERY_LENGTH 8192

void unlock_func(void *data);

class Sqlite3Storage : public Storage
{
public:
    Sqlite3Storage();
    virtual ~Sqlite3Storage();

    virtual void init(zmm::Ref<Dictionary> params);    
    virtual void addObject(zmm::Ref<CdsObject> object);
    virtual void updateObject(zmm::Ref<CdsObject> object);
    virtual void eraseObject(zmm::Ref<CdsObject> object);

    virtual zmm::Ref<zmm::Array<CdsObject> > browse(zmm::Ref<BrowseParam> param);
    virtual zmm::Ref<zmm::Array<zmm::StringBase> > getMimeTypes(); 
    virtual zmm::Ref<CdsObject> findObjectByTitle(zmm::String title, zmm::String parentID);

    // utility methods
    virtual void removeObject(zmm::Ref<CdsObject> object);
    
protected:

    pthread_mutex_t lock_mutex;
    sqlite3 *db;

    char **table;
    char **row;

    int cur_row;
    
    int nrow;
    int ncolumn;

    void select(zmm::String query);
    int nextRow();
    void finish();

    void exec(zmm::String query);
    
    void reportError(zmm::String query);
    zmm::Ref<CdsObject> createObjectFromRow();
    
    void removeChildren(zmm::Ref<CdsObject> object, zmm::Ref<zmm::StringBuffer> query);
    void lock();
    void unlock();

    friend void unlock_func(void *data);
};


#endif // __SQLITE3_STORAGE_H__

