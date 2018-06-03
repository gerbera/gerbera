/*GRB*

Gerbera - https://gerbera.io/

    http_protocol_helper.h - this file is part of Gerbera.

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

/// \file http_protocol_helper.h

#ifndef GERBERA_HTTP_PROTOCOL_HELPER_H
#define GERBERA_HTTP_PROTOCOL_HELPER_H

class HttpProtocolHelper {
 public:
  HttpProtocolHelper();
  ~HttpProtocolHelper();

  /// \brief Adds carriage-return + line-feed [ \r\n ] to end of header (if needed).
  /// \return string representing finalized header with `/r/n`
  std::string finalizeHttpHeader(std::string header);
};

#endif //GERBERA_HTTP_PROTOCOL_HELPER_H
