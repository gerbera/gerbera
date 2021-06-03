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

#include "session_manager.h" // API

#include <memory>
#include <unordered_set>

#include "config/config_manager.h"
#include "util/timer.h"
#include "util/tools.h"

#define UI_UPDATE_ID_HASH_SIZE 61
#define MAX_UI_UPDATE_IDS 10

namespace web {

Session::Session(std::chrono::seconds timeout)
    : uiUpdateIDs(std::make_shared<std::unordered_set<int>>())
    , timeout(timeout)
{
    access();
}

void Session::put(const std::string& key, std::string value)
{
    AutoLockR lock(rmutex);
    dict[key] = std::move(value);
}

std::string Session::get(const std::string& key)
{
    AutoLockR lock(rmutex);
    return getValueOrDefault(dict, key);
}

void Session::containerChangedUI(int objectID)
{
    if (objectID == INVALID_OBJECT_ID)
        return;
    if (!updateAll) {
        AutoLockR lock(rmutex);
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
    AutoLockR lock(rmutex);

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
    AutoLockR lock(rmutex);
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
    AutoLockR lock(rmutex);
    uiUpdateIDs->clear();
    updateAll = false;
}

SessionManager::SessionManager(const std::shared_ptr<Config>& config, std::shared_ptr<Timer> timer)
    : timer(std::move(timer))
    , timerAdded(false)
{
    accounts = config->getDictionaryOption(CFG_SERVER_UI_ACCOUNT_LIST);
}

std::shared_ptr<Session> SessionManager::createSession(std::chrono::seconds timeout)
{
    auto newSession = std::make_shared<Session>(timeout);
    AutoLock lock(mutex);

    int count = 0;
    std::string sessionID;
    do {
        sessionID = generateRandomId();
        if (count++ > 100)
            throw_std_runtime_error("There seems to be something wrong with the random numbers. I tried to get a unique id 100 times and failed. last sessionID: {}", sessionID);
    } while (getSession(sessionID, false) != nullptr); // for the rare case, where we get a random id, that is already taken

    newSession->setID(sessionID);
    sessions.push_back(newSession);
    checkTimer();
    return newSession;
}

std::shared_ptr<Session> SessionManager::getSession(const std::string& sessionID, bool doLock)
{
    if (sessions.empty()) {
        return nullptr;
    }

    auto lock = std::unique_lock<std::mutex>(mutex, std::defer_lock);
    if (doLock)
        lock.lock();

    auto it = std::find_if(sessions.begin(), sessions.end(), [&](auto&& s) { return s->getID() == sessionID; });
    return it != sessions.end() ? *it : nullptr;
}

void SessionManager::removeSession(const std::string& sessionID)
{
    if (sessions.empty()) {
        return;
    }

    AutoLock lock(mutex);

    auto sess = std::find_if(sessions.begin(), sessions.end(), [&](auto&& s) { return s->getID() == sessionID; });
    if (sess != sessions.end()) {
        sess = sessions.erase(sess);
        checkTimer();
        return;
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
    for (auto&& session : sessions) {
        if (session->isLoggedIn())
            session->containerChangedUI(objectID);
    }
}

void SessionManager::containerChangedUI(const std::vector<int>& objectIDs)
{
    if (sessions.empty())
        return;
    AutoLock lock(mutex);
    for (auto&& session : sessions) {
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

    auto now = currentTimeMS();

    for (auto it = sessions.begin(); it != sessions.end(); /*++it*/) {
        auto&& session = *it;

        if (getDeltaMillis(session->getLastAccessTime(), now) > session->getTimeout()) {
            log_debug("session timeout: {} - diff: {}", session->getID().c_str(), getDeltaMillis(session->getLastAccessTime(), now).count());
            it = sessions.erase(it);
            checkTimer();
        } else
            ++it;
    }
}

} // namespace web
