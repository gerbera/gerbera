/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    thread_executor.cc - this file is part of MediaTomb.
    
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

/// \file thread_executor.cc

#include "thread_executor.h"

using namespace zmm;

ThreadExecutor::ThreadExecutor()
{
    mutex = Ref<Mutex>(new Mutex());
    cond = Ref<Cond>(new Cond(mutex));
    
    threadShutdown = false;
}

ThreadExecutor::~ThreadExecutor()
{
    kill();
}

void ThreadExecutor::startThread()
{
    threadRunning = true;
    pthread_create(
        &thread,
        NULL, // attr
        ThreadExecutor::staticThreadProc,
        this
    );
}

bool ThreadExecutor::kill()
{
    if (! threadRunning)
        return true;
    AUTOLOCK(mutex);
    threadShutdown = true;
    cond->signal();
    AUTOUNLOCK();
    if (thread)
    {
        threadRunning = false;
        pthread_join(thread, NULL);
    }
    return true;
}

void *ThreadExecutor::staticThreadProc(void *arg)
{
    ThreadExecutor *inst = (ThreadExecutor *)arg;
    inst->threadProc();
    pthread_exit(NULL);
    return NULL;
}
