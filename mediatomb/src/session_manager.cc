/*  session_manager.cc - this file is part of MediaTomb.
                                                                                
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

#include "uuid/uuid.h"
#include "session_manager.h"
#include "tools.h"

using namespace zmm;

static Ref<SessionManager> inst;

Ref<SessionManager> SessionManager::getInstance()
{
    if (inst == nil)
        inst = Ref<SessionManager>(new SessionManager());
    return inst;
}

Session::Session(long timeout) : Dictionary()
{
    driver1 = Ref<Dictionary>(new Dictionary());
    driver2 = Ref<Dictionary>(new Dictionary());

    this->timeout = timeout;
}

void Session::putTo(session_data_t type, String key, String value)
{
    switch (type)
    {
        case PRIMARY:
            driver1->put(key, value);
            break;
        case SECONDARY:
            driver2->put(key, value);
            break;
        default:
            throw Exception(_("Unknown session data type: ") + (int)type);
    }
}

String Session::getFrom(session_data_t type, String key)
{
    switch (type)
    {
        case PRIMARY:
            return driver1->get(key);
        case SECONDARY:
            return driver2->get(key);
        default:
            throw Exception(_("Unknown session data type: ") + (int)type);
    }
}

SessionManager::SessionManager() : Object()
{
     sessions = Ref<Array<Session> >(new Array<Session>());
}

Ref<Session> SessionManager::createSession(long timeout, String sessionID)
{
    Ref<Session> newSession(new Session(timeout));
    if (sessionID == nil)
        sessionID = generate_random_id();
    newSession->setID(sessionID);
    sessions->append(newSession);
    return newSession;
}

Ref<Session> SessionManager::getSession(String sessionID)
{
    for (int i = 0; i < sessions->size(); i++)
    {
        Ref<Session> s = sessions->get(i);
        if (s->getID() == sessionID)
            return s;
    }

    return nil;
}


