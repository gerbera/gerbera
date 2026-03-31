/*GRB*

    Gerbera - https://gerbera.io/

    cuesheet_parser_script.h - this file is part of Gerbera.

    Copyright (C) 2026 Gerbera Contributors

    Gerbera is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.

    Gerbera is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Gerbera.  If not, see <http://www.gnu.org/licenses/>.

    $Id$
*/

/// @file content/scripting/cuesheet_parser_script.h

#ifndef __CUESHEET_PARSER_SCRIPT_H__
#define __CUESHEET_PARSER_SCRIPT_H__

#include "parser_script.h"

class CuesheetParserScript : public ParserScript {
public:
    CuesheetParserScript(const std::shared_ptr<Content>& content, const std::string& parent);
    std::vector<int> processObject(
        const std::shared_ptr<CdsObject>& obj,
        const fs::path& path);

    std::pair<std::shared_ptr<CdsObject>, int> createObject2cdsObject(
        const std::shared_ptr<CdsObject>& origObject,
        const std::string& rootPath) override;
    bool setRefId(
        const std::shared_ptr<CdsObject>& cdsObj,
        const std::shared_ptr<CdsObject>& origObject,
        int pcdId) override;

protected:
    std::string cuesheetFunction;

    void handleObject2cdsItem(
        duk_context* ctx,
        const std::shared_ptr<CdsObject>& pcd,
        const std::shared_ptr<CdsItem>& item) override;
    void handleObject2cdsContainer(
        duk_context* ctx,
        const std::shared_ptr<CdsObject>& pcd,
        const std::shared_ptr<CdsContainer>& cont) override;
};

#endif // __CUESHEET_PARSER_SCRIPT_H__
