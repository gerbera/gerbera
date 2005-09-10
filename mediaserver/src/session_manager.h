/*  session_manager.h - this file is part of MediaTomb.
                                                                                
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

/// \file session_manager.h
/// \brief Definitions of the Session and SessionManager classes.
#ifndef __SESSION_MANAGER_H__
#define __SESSION_MANAGER_H__

#include "dictionary.h"

/// \brief Each session has two separate information placeholders, one for each driver.
typedef enum
{
    PRIMARY   = 1,
    SECONDARY = 2
} session_data_t;

/// \brief One UI session.
/// 
/// When the user logs in for the first time (via the web UI) a new Session will be
/// created. It then will be used to store various information to support
/// the UI - remember the position of browsing for each driver, remember the page
/// count for each driver and so on.
class Session : public Dictionary 
{
public:
    /// \brief Constructor, creates a session with a given timeout.
    /// \param timeout time in milliseconds after which the session will expire if not accessed.
    ///
    /// The session is created with a given timeout, each access to the session updates the
    /// last_access value, if last access lies further back than the timeout - the session will
    /// be deleted (will time out)
    /// \todo timeouts and session purging not yet implemented
    Session(long timeout);

    /// \brief Returns the time of last access to the session.
    /// \return time in milliseconds
    /// \todo what time is it? from epoch? not yet implemented
    inline long getLastAccessTime() { return last_access; }
    
    /// \brief Returns the session identifier.
    inline zmm::String getID() { return sessionID; }

    /// \brief Sets the session identifier.
    inline void setID(zmm::String sessionID) { this->sessionID = sessionID; }

    /// \brief Saves a value in a specific placeholder.
    /// \param type identifies in which placeholder the data will be saved
    /// \param key the key under which the data can be accessed
    /// \param value a value for the given key
    ///
    /// Basically this is not different than saving key:value pairs in a 
    /// dictionary. The session itself is derived from the Dictionary class,
    /// but inside the session we have two more separate dictionaries.
    /// This is required, because we will often need to save different data 
    /// (for each driver) with same key names
    void putTo(session_data_t type, zmm::String key, zmm::String value);

    /// \brief Retrieve previously saved data from a specific placeholder.
    /// \param type identifies from which placeholder the data will be retrieved
    /// \param key the value for this key will be returned
    /// \return value as String
    ///
    /// This is same as normal "get()" from the Dictionary class, the
    /// only difference is, that we here identifiy the dictionary by the
    /// driver (see description for putTo())
    zmm::String getFrom(session_data_t type, zmm::String key);

protected:
    /// \brief maximum time the session can be idle (starting from last_access)
    long timeout;

    /// \brief time of last access to the session, returned by getLastAccessTime()
    long last_access;

    /// \brief arbitrary but unique string representing the ID of the session (returned by getID())
    zmm::String sessionID;

    /// \brief separate placeholder for storing primary driver session info (see putTo() and getFrom())
    zmm::Ref<Dictionary> driver1;
    
    /// \brief separate placeholder for storing secondary driver session info (see putTo() and getFrom())
    zmm::Ref<Dictionary> driver2;
};

/// \brief This class offers ways to create new sessoins, stores all available sessions and provides access to them.
class SessionManager : public zmm::Object
{
protected:
    /// \brief This array is holding available sessions.
    zmm::Ref<zmm::Array<Session> > sessions;
    
public:
    /// \brief Constructor, initializes the array.
    SessionManager();

    /// \brief Returns the SessionManager instance or creates a new SessionManager if called for the first time.
    /// \return instance of the SessionManager
    ///
    /// The SessionManager should only be instantiated once in the whole application, this method 
    /// provides the means to accomplish it. When called for the first time a new SessionManager will
    /// be created, otherwise the already existing instance will be returned.
    static zmm::Ref<SessionManager> getInstance();

    /// \brief Creates a Session with a given timeout.
    /// \param timeout Session timeout in milliseconds.
    zmm::Ref<Session> createSession(long timeout, zmm::String sessionID = nil);

    /// \brief Returns the instance to a Session with a given sessionID
    /// \param ID of the Session.
    /// \return intance of the Session with a given ID or nil if no session with that ID was found.
    zmm::Ref<Session> getSession(zmm::String sessionID);
};

#endif //  __SESSION_MANAGER_H__

