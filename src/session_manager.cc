/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    session_manager.cc - this file is part of MediaTomb.
    
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

/// \file session_manager.cc

#include <memory>
#include <unordered_set>

#include "session_manager.h"
#include "config_manager.h"
#include "tools.h"
#include "tools.h"
#include "timer.h"

#define UI_UPDATE_ID_HASH_SIZE  61
#define MAX_UI_UPDATE_IDS       10

using namespace zmm;
using namespace mxml;
using namespace std;

Session::Session(long timeout) : Dictionary_r()
{
    this->timeout = timeout;
    loggedIn = false;
    sessionID = nullptr;
    uiUpdateIDs = make_shared<unordered_set<int>>();
        //(new DBRHash<int>(UI_UPDATE_ID_HASH_SIZE, MAX_UI_UPDATE_IDS + 5, INVALID_OBJECT_ID, INVALID_OBJECT_ID_2));
    updateAll = false;
    access();
}

void Session::containerChangedUI(int objectID)
{
    if (objectID == INVALID_OBJECT_ID)
        return;
    if (! updateAll)
    {
        AutoLock lock(mutex);
        if (! updateAll)
        {
            if (uiUpdateIDs->size() >= MAX_UI_UPDATE_IDS)
            {
                updateAll = true;
                uiUpdateIDs->clear();
            }
            else
                uiUpdateIDs->insert(objectID);
        }
    }
}

void Session::containerChangedUI(Ref<IntArray> objectIDs)
{
    if (updateAll)
        return;
    if (objectIDs == nullptr)
        return;
    int arSize = objectIDs->size();
    AutoLock lock(mutex);
    if (updateAll)
        return;
    if (uiUpdateIDs->size() + arSize >= MAX_UI_UPDATE_IDS)
    {
        updateAll = true;
        uiUpdateIDs->clear();
        return;
    }
    for (int i = 0; i < arSize; i++)
    {
        uiUpdateIDs->insert(objectIDs->get(i));
    }
}

String Session::getUIUpdateIDs()
{
    if (! hasUIUpdateIDs())
        return nullptr;
    AutoLock lock(mutex);
    if (updateAll)
    {
        updateAll = false;
        return _("all");
    }
    String ret = toCSV(uiUpdateIDs);
    if (ret != nullptr)
        uiUpdateIDs->clear();
    return ret;
}

bool Session::hasUIUpdateIDs()
{
    if (updateAll)
        return true;
    // AutoLock lock(mutex); only accessing an int - shouldn't be necessary
    return (uiUpdateIDs->size() > 0);
}

void Session::clearUpdateIDs()
{
    log_debug("clearing UI updateIDs\n");
    AutoLock lock(mutex);
    uiUpdateIDs->clear();
    updateAll = false;
}

SessionManager::SessionManager()
{
    Ref<ConfigManager> configManager = ConfigManager::getInstance();

    accounts = configManager->getDictionaryOption(CFG_SERVER_UI_ACCOUNT_LIST);
    sessions = Ref<Array<Session> >(new Array<Session>());
    timerAdded = false;
}

Ref<Session> SessionManager::createSession(long timeout)
{
    Ref<Session> newSession(new Session(timeout));
    AutoLock lock(mutex);
    
    int count=0;
    String sessionID;
    do
    {
        sessionID = generate_random_id();
        if (count++ > 100)
            throw _Exception(_("There seems to be something wrong with the random numbers. I tried to get a unique id 100 times and failed. last sessionID: ") + sessionID);
    }
    while(getSession(sessionID, false) != nullptr); // for the rare case, where we get a random id, that is already taken
    
    newSession->setID(sessionID);
    sessions->append(newSession);
    checkTimer();
    return newSession;
}

Ref<Session> SessionManager::getSession(String sessionID, bool doLock)
{
    unique_lock<decltype(mutex)> lock(mutex, std::defer_lock);
    if (doLock)
        lock.lock();
    for (int i = 0; i < sessions->size(); i++)
    {
        Ref<Session> s = sessions->get(i);
        if (s->getID() == sessionID)
            return s;
    }
    return nullptr;
}

void SessionManager::removeSession(String sessionID)
{
    AutoLock lock(mutex);
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
    if (accounts == nullptr)
    {
        return nullptr;
    }
    return accounts->get(user);
}

void SessionManager::containerChangedUI(int objectID)
{
    if (sessions->size() <= 0)
        return;
    AutoLock lock(mutex);
    int sesSize = sessions->size();
    for (int i = 0; i < sesSize; i++)
    {
        Ref<Session> session = sessions->get(i);
        if (session->isLoggedIn())
            session->containerChangedUI(objectID);
    }
}

void SessionManager::containerChangedUI(Ref<IntArray> objectIDs)
{
    if (sessions->size() <= 0)
        return;
    AutoLock lock(mutex);
    int sesSize = sessions->size();
    for (int i = 0; i < sesSize; i++)
    {
        Ref<Session> session = sessions->get(i);
        if (session->isLoggedIn())
            session->containerChangedUI(objectIDs);
    }
}

void SessionManager::checkTimer()
{
    if (sessions->size() > 0 && ! timerAdded)
    {
        Timer::getInstance()->addTimerSubscriber(this, SESSION_TIMEOUT_CHECK_INTERVAL);
        timerAdded = true;
    }
    else if (sessions->size() <= 0 && timerAdded)
    {
        Timer::getInstance()->removeTimerSubscriber(this);
        timerAdded = false;
    }
}

void SessionManager::timerNotify(Ref<Object> parameter)
{
    log_debug("notified... %d sessions.\n", sessions->size());
    AutoLock lock(mutex);
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
