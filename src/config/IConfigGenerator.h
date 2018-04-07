/*GRB*

Gerbera - https://gerbera.io/

    IConfigGenerator.h - this file is part of Gerbera.

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

/// \file IConfigGenerator.h

#ifndef GERBERA_ICONFIGGENERATOR_H
#define GERBERA_ICONFIGGENERATOR_H

class IConfigGenerator {

 public:
  virtual ~IConfigGenerator() {}
  virtual std::string generate(std::string userHome, std::string configDir, std::string prefixDir, std::string magicFile) = 0;
};

#endif //GERBERA_ICONFIGGENERATOR_H
