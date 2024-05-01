/*GRB*

    Gerbera - https://gerbera.io/

    ui_handler.h - this file is part of Gerbera.

    Copyright (C) 2024 Gerbera Contributors

    Gerbera is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.

    Gerbera is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Gerbera.  If not, see <http://www.gnu.org/licenses/>.
*/

/// \file ui_handler.h

#ifndef GERBERA_UI_HANDLER_H
#define GERBERA_UI_HANDLER_H

#include "request_handler.h"

#include <memory>

class UpnpXMLBuilder;

class UIHandler : public RequestHandler {
public:
    explicit UIHandler(const std::shared_ptr<ContentManager>& content, const std::shared_ptr<UpnpXMLBuilder>& xmlBuilder);

    void getInfo(const char* filename, UpnpFileInfo* info) override;
    std::unique_ptr<IOHandler> open(const char* filename, enum UpnpOpenFileMode mode) override;
};

#endif // GERBERA_UI_HANDLER_H
