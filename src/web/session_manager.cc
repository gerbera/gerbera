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
#include <utility>

#include "config/config_manager.h"
#include "session_manager.h"
#include "util/timer.h"
#include "util/tools.h"

#define UI_UPDATE_ID_HASH_SIZE 61
#define MAX_UI_UPDATE_IDS 10

namespace web {

Session::Session(long timeout)
{
    this->timeout = timeout;
    loggedIn = false;
    sessionID = "";
    uiUpdateIDs = std::make_shared<std::unordered_set<int>>();
    //(new DBRHash<int>(UI_UPDATE_ID_HASH_SIZE, MAX_UI_UPDATE_IDS + 5, INVALID_OBJECT_ID, INVALID_OBJECT_ID_2));
    updateAll = false;
    access();
}

void Session::put(const std::string& key, std::string value)
{
    AutoLock lock(mutex);
    dict[key] = std::move(value);
}

std::string Session::get(const std::string& key)
{
    AutoLock lock(mutex);
    return getValueOrDefault(dict, key);
}

void Session::containerChangedUI(int objectID)
{
    if (objectID == INVALID_OBJECT_ID)
        return;
    if (!updateAll) {
        AutoLock lock(mutex);
        if (!updateAll) {
            if (uiUpdateIDs->size() >= MAX_UI_UPDATE_IDS) {
                updateAll = true;
                uiUpdateIDs->clear();
            } else
                uiUpdateIDs->insert(objectID);
        }
    }
}

void Session::containerChangedUI(const std::vector<int>& objectIDs)
{
    if (updateAll)
        return;

    size_t arSize = objectIDs.size();
    AutoLock lock(mutex);

    if (updateAll)
        return;

    if (uiUpdateIDs->size() + arSize >= MAX_UI_UPDATE_IDS) {
        updateAll = true;
        uiUpdateIDs->clear();
        return;
    }
    for (int objectId : objectIDs) {
        uiUpdateIDs->insert(objectId);
    }
}

std::string Session::getUIUpdateIDs()
{
    if (!hasUIUpdateIDs())
        return "";
    AutoLock lock(mutex);
    if (updateAll) {
        updateAll = false;
        return "all";
    }
    std::string ret = toCSV(uiUpdateIDs);
    if (!ret.empty())
        uiUpdateIDs->clear();
    return ret;
}

bool Session::hasUIUpdateIDs() const
{
    if (updateAll)
        return true;
    // AutoLock lock(mutex); only accessing an int - shouldn't be necessary
    return (!uiUpdateIDs->empty());
}

void Session::clearUpdateIDs()
{
    log_debug("clearing UI updateIDs");
    AutoLock lock(mutex);
    uiUpdateIDs->clear();
    updateAll = false;
}

SessionManager::SessionManager(const std::shared_ptr<ConfigManager>& config, std::shared_ptr<Timer> timer)
{
    this->timer = std::move(timer);
    accounts = config->getDictionaryOption(CFG_SERVER_UI_ACCOUNT_LIST);
    timerAdded = false;
}

std::shared_ptr<Session> SessionManager::createSession(long timeout)
{
    auto newSession = std::make_shared<Session>(timeout);
    AutoLock lock(mutex);

    int count = 0;
    std::string sessionID;
    do {
        sessionID = generate_random_id();
        if (count++ > 100)
            throw std::runtime_error("There seems to be something wrong with the random numbers. I tried to get a unique id 100 times and failed. last sessionID: " + sessionID);
    } while (getSession(sessionID, false) != nullptr); // for the rare case, where we get a random id, that is already taken

    newSession->setID(sessionID);
    sessions.push_back(newSession);
    checkTimer();
    return newSession;
}

std::shared_ptr<Session> SessionManager::getSession(const std::string& sessionID, bool doLock)
{
    std::unique_lock<decltype(mutex)> lock(mutex, std::defer_lock);
    if (doLock)
        lock.lock();
    for (const auto& s : sessions) {
        if (s->getID() == sessionID)
            return s;
    }
    return nullptr;
}

void SessionManager::removeSession(const std::string& sessionID)
{
    AutoLock lock(mutex);

    for (auto it = sessions.begin(); it != sessions.end(); /*++it*/) {
        auto& s = *it;

        if (s->getID() == sessionID) {
            it = sessions.erase(it);
            checkTimer();
            return;
        }
        ++it;
    }
}

std::string SessionManager::getUserPassword(const std::string& user)
{
    return getValueOrDefault(accounts, user);
}

void SessionManager::containerChangedUI(int objectID)
{
    if (sessions.empty())
        return;
    AutoLock lock(mutex);
    for (const auto& session : sessions) {
        if (session->isLoggedIn())
            session->containerChangedUI(objectID);
    }
}

void SessionManager::containerChangedUI(const std::vector<int>& objectIDs)
{
    if (sessions.empty())
        return;
    AutoLock lock(mutex);
    for (const auto& session : sessions) {
        if (session->isLoggedIn())
            session->containerChangedUI(objectIDs);
    }
}

void SessionManager::checkTimer()
{
    if (!sessions.empty() && !timerAdded) {
        timer->addTimerSubscriber(this, SESSION_TIMEOUT_CHECK_INTERVAL);
        timerAdded = true;
    } else if (sessions.empty() && timerAdded) {
        timer->removeTimerSubscriber(this);
        timerAdded = false;
    }
}

void SessionManager::timerNotify(std::shared_ptr<Timer::Parameter> parameter)
{
    log_debug("notified... {} web sessions.", sessions.size());

    AutoLock lock(mutex);

    struct timespec now;
    getTimespecNow(&now);

    for (auto it = sessions.begin(); it != sessions.end(); /*++it*/) {
        const auto& session = *it;

        if (getDeltaMillis(session->getLastAccessTime(), &now) > 1000 * session->getTimeout()) {
            log_debug("session timeout: {} - diff: {}", session->getID().c_str(), getDeltaMillis(session->getLastAccessTime(), &now));
            it = sessions.erase(it);
            checkTimer();
        } else
            ++it;
    }
}

} // namespace web
