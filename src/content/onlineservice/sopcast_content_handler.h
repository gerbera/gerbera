/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    sopcast_content_handler.h - this file is part of MediaTomb.
    
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

/// \file sopcast_content_handler.h
/// \brief Definitions of the SopCastContentHandler class.

#ifdef SOPCAST

#ifndef __SOPCAST_CONTENT_HANDLER_H__
#define __SOPCAST_CONTENT_HANDLER_H__

#include <memory>
#include <pugixml.hpp>

#include "context.h"
#include "curl_online_service.h"

#define SOPCAST_SERVICE "SopCast"
#define SOPCAST_SERVICE_ID "S"
#define SOPCAST_CHANNEL_ID "cid"

#define SOPCAST_PROTOCOL "sop"

#define SOPCAST_AUXDATA_LANGUAGE SOPCAST_SERVICE_ID "1"
#define SOPCAST_AUXDATA_GROUP SOPCAST_SERVICE_ID "2"

// forward declaration
class CdsObject;

/// \brief this class is responsible for creating objects from the SopCast
/// metadata XML.
class SopCastContentHandler : public CurlContentHandler {
public:
    explicit SopCastContentHandler(const std::shared_ptr<Context>& context);

    /// \brief Sets the service XML from which we will extract the objects.
    /// \return false if service XML contained an error status.
    void setServiceContent(std::unique_ptr<pugi::xml_document> service) override;

    /// \brief retrieves an object from the service.
    ///
    /// Each invokation of this funtion will return a new object,
    /// when the whole service XML is parsed and no more objects are left,
    /// this function will return nullptr.
    ///
    /// \return CdsObject or nullptr if there are no more objects to parse.
    std::shared_ptr<CdsObject> getNextObject() override;

protected:
    static std::shared_ptr<CdsObject> getObject(const std::string& groupName, const pugi::xml_node& channel);

protected:
    std::shared_ptr<Config> config;
    std::shared_ptr<Database> database;

    std::unique_ptr<pugi::xml_document> service_xml;
    pugi::xml_node_iterator group_it;
    pugi::xml_node_iterator channel_it;
};

#endif //__SOPCAST_CONTENT_HANDLER_H__

#endif //SOPCAST
