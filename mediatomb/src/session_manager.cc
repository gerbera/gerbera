/*MT*
    
    MediaTomb - http://www.mediatomb.org/
    
    session_manager.cc - this file is part of MediaTomb.
    
    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.org>,
                       Sergey Bostandzhyan <jin@mediatomb.org>
    Copyright (C) 2006 Gena Batyan <bgeradz@mediatomb.org>,
                       Sergey Bostandzhyan <jin@mediatomb.org>,
                       Leonhard Wimmer <leo@mediatomb.org>
    
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
    
    $Id$
*/

/// \file session_manager.cc

#ifdef HAVE_CONFIG_H
    #include "autoconfig.h"
#endif

#include "uuid/uuid.h"
#include "session_manager.h"
#include "config_manager.h"
#include "tools.h"
#include "tools.h"
#include "timer.h"

#define UI_UPDATE_ID_HASH_SIZE  61
#define MAX_UI_UPDATE_IDS       10

using namespace zmm;
using namespace mxml;

static Ref<SessionManager> inst;

Ref<SessionManager> SessionManager::getInstance()
{
    if (inst == nil)
    {
        AUTOLOCK(mutex);
        if (inst == nil)
        {
            try
            {
                inst = Ref<SessionManager>(new SessionManager());
            }
            catch (Exception e)
            {
                throw e;
            }
        }
    }
    return inst;
}

Session::Session(long timeout) : Dictionary_r()
{
    this->timeout = timeout;
    loggedIn = false;
    sessionID = nil;
    uiUpdateIDs = Ref<DBRHash<int> >(new DBRHash<int>(UI_UPDATE_ID_HASH_SIZE, INVALID_OBJECT_ID, INVALID_OBJECT_ID_2));
    updateAll = false;
    access();
}

void Session::containerChangedUI(int objectID)
{
    if (objectID == INVALID_OBJECT_ID)
        return;
    if (! updateAll)
    {
        AUTOLOCK(mutex);
        if (! updateAll)
        {
            if (uiUpdateIDs->size() >= MAX_UI_UPDATE_IDS)
            {
                updateAll = true;
                uiUpdateIDs->clear();
            }
            else
                uiUpdateIDs->put(objectID);
        }
    }
}

String Session::getUIUpdateIDs()
{
    AUTOLOCK(mutex);
    if (updateAll)
    {
        updateAll = false;
        return _("all");
    }
    hash_data_array_t<int> hash_data_array;
    uiUpdateIDs->getAll(&hash_data_array);
    String ret = intArrayToCSV(hash_data_array.data, hash_data_array.size);
    if (ret != nil)
        uiUpdateIDs->clear();
    return ret;
}

Ref<Mutex> SessionManager::mutex(new Mutex());

SessionManager::SessionManager() : TimerSubscriber()
{
    Ref<ConfigManager> configManager = ConfigManager::getInstance();
    Ref<Element> accountsEl = configManager->getElement(_("/server/ui/accounts"));
    if (accountsEl == nil)
    {
        log_debug("accounts tag not found in config\n");
        accounts = Ref<Dictionary>(new Dictionary());
    }
    else
    {
        accounts = configManager->createDictionaryFromNodeset(accountsEl, _("account"), _("user"), _("password"));
    }
    sessions = Ref<Array<Session> >(new Array<Session>());
    timerAdded = false;
}

Ref<Session> SessionManager::createSession(long timeout)
{
    Ref<Session> newSession(new Session(timeout));
    AUTOLOCK(mutex);
    
    int count=0;
    String sessionID;
    do
    {
        sessionID = generate_random_id();
        if (count++ > 100)
            throw _Exception(_("There seems to be something wrong with the random numbers. I tried to get a unique id 100 times and failed. last sessionID: ") + sessionID);
    }
    while(getSession(sessionID, false) != nil); // for the rare case, where we get a random id, that is already taken
    
    newSession->setID(sessionID);
    sessions->append(newSession);
    checkTimer();
    return newSession;
}

Ref<Session> SessionManager::getSession(String sessionID, bool doLock)
{
    AUTOLOCK_NOLOCK(mutex)
    if (doLock)
        AUTORELOCK();
    for (int i = 0; i < sessions->size(); i++)
    {
        Ref<Session> s = sessions->get(i);
        if (s->getID() == sessionID)
            return s;
    }
    return nil;
}

void SessionManager::removeSession(String sessionID)
{
    AUTOLOCK(mutex);
    for (int i = 0; i < sessions->size(); i++)
    {
        Ref<Session> s = sessions->get(i);
        if (s->getID() == sessionID)
        {
            sessions->remove(i);
            checkTimer();
            i--; // to not skip a session. the removed id is now taken by another session
            return;
        }
    }
}

String SessionManager::getUserPassword(String user)
{
    if (accounts == nil)
    {
        return nil;
    }
    return accounts->get(user);
}

void SessionManager::containerChangedUI(int objectID)
{
    AUTOLOCK(mutex);
    for (int i = 0; i < sessions->size(); i++)
    {
        Ref<Session> session = sessions->get(i);
        if (session->isLoggedIn())
            session->containerChangedUI(objectID);
    }
}

void SessionManager::checkTimer()
{
    if (sessions->size() > 0 && ! timerAdded)
    {
        Timer::getInstance()->addTimerSubscriber(RefCast(inst, TimerSubscriber), SESSION_TIMEOUT_CHECK_INTERVAL);
        timerAdded = true;
    }
    else if (sessions->size() <= 0 && timerAdded)
    {
        Timer::getInstance()->removeTimerSubscriber(RefCast(inst, TimerSubscriber));
        timerAdded = false;
    }
}

void SessionManager::timerNotify(int id)
{
    log_debug("notified... %d sessions.\n", sessions->size());
    AUTOLOCK(mutex);
    struct timespec now;
    getTimespecNow(&now);
    for (int i = 0; i < sessions->size(); i++)
    {
        Ref<Session> session = sessions->get(i);
        if (getDeltaMillis(session->getLastAccessTime(), &now) > 1000 * session->getTimeout())
        {
            log_debug("session timeout: %s - diff: %ld\n", session->getID().c_str(), getDeltaMillis(session->getLastAccessTime(), &now));
            sessions->remove(i);
            checkTimer();
            i--; // to not skip a session. the removed id is now taken by another session
        }
    }
}
