/*GRB*

Gerbera - https://gerbera.io/

    http_protocol_helper.cc - this file is part of Gerbera.

    Copyright (C) 2016-2018 Gerbera Contributors

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

/// \file http_protocol_helper.cc

#include <string>
#include "http_protocol_helper.h"

HttpProtocolHelper::HttpProtocolHelper() {}
HttpProtocolHelper::~HttpProtocolHelper() {}

std::string HttpProtocolHelper::finalizeHttpHeader(std::string header) {
  std::string result = header;
  if(header.length() > 0) {
    std::size_t found = header.find_last_of("\r\n");
    if (found == std::string::npos) {
      result = result + "\r\n";
    } else if (found < header.length() - 1) {
      result = result + "\r\n";
    }
  }
  return result;
}