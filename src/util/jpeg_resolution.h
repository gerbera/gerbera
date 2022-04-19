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
