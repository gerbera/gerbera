/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    fileinput.h - this file is part of MediaTomb.
    
    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>
    
    Copyright (C) 2006-2008 Gena Batyan <bgeradz@mediatomb.cc>,
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

/// \file fileinput.h

#ifndef __MXML_FILEINPUT_H__
#define __MXML_FILEINPUT_H__

#include "zmmf/zmmf.h"
#include "input.h"
#include <stdio.h>


namespace mxml
{

class FileInput : public Input
{
protected:
    FILE *file;
public:
    FileInput(zmm::String filename);
    virtual ~FileInput();
    virtual int readChar();
};

} // namespace

#endif // __MXML_FILEINPUT_H__
