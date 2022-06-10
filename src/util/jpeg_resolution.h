/*GRB*

    Gerbera - https://gerbera.io/

    jpeg_resolution.h - this file is part of Gerbera.

    Copyright (C) 2022 Gerbera Contributors

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

//
// Created by Ian Whyman on 19/04/2022.
//

#ifndef GERBERA_JPEG_RESOLUTION_H
#define GERBERA_JPEG_RESOLUTION_H

#include "metadata/resolution.h"
class IOHandler;

/// \brief Extracts resolution from a JPEG image
/// \param ioh the IOHandler must be opened. The function will read data and close the handler.
Resolution getJpegResolution(IOHandler& ioh);

#endif // GERBERA_JPEG_RESOLUTION_H
