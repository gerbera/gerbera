/*MT*

    MediaTomb - http://www.mediatomb.cc/

    atrailers_content_handler.h - this file is part of MediaTomb.

    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>

    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>

    Copyright (C) 2016-2024 Gerbera Contributors

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

/// \file atrailers_content_handler.h
/// \brief Definitions of the ATrailersContentHandler class.

#ifndef __ATRAILERS_CONTENT_HANDLER_H__
#define __ATRAILERS_CONTENT_HANDLER_H__

#ifdef ATRAILERS

#include "curl_online_service.h"

// forward declaration
class CdsObject;

#define ATRAILERS_SERVICE "Apple Trailers"
#define ATRAILERS_SERVICE_ID "T"

#define ATRAILERS_AUXDATA_POST_DATE ATRAILERS_SERVICE_ID "0"

/// \brief this class is responsible for creating objects from the ATrailers
/// metadata XML.
class ATrailersContentHandler : public CurlContentHandler {
    using CurlContentHandler::CurlContentHandler;

public:
    /// \brief Sets the service XML from which we will extract the objects.
    /// \return false if service XML contained an error status.
    void setServiceContent(std::unique_ptr<pugi::xml_document> service) override;

    /// \brief retrieves an object from the service.
    ///
    /// Each invokation of this funtion will return a new object,
    /// when the whole service XML is parsed and no more objects are left,
    /// this function will return nullptr.
    ///
    /// \return \c CdsObject or \c nullptr if there are no more objects to parse.
    std::shared_ptr<CdsObject> getNextObject() override;

    /// \brief Checks whether we reached the end of the file
    bool isEnd() override;

protected:
    std::shared_ptr<CdsObject> getObject(const pugi::xml_node& trailer) const;

    pugi::xml_node_iterator trailer_it;
    pugi::xml_node root;

    std::string trailer_mimetype;
};

#endif // ATRAILERS

#endif //__ATRAILERS_CONTENT_HANDLER_H__
