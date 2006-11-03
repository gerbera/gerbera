/*MT*
    
    MediaTomb - http://www.mediatomb.org/
    
    stringinput.h - this file is part of MediaTomb.
    
    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.org>,
                       Sergey Bostandzhyan <jin@mediatomb.org>
    Copyright (C) 2006 Gena Batyan <bgeradz@mediatomb.org>,
                       Sergey Bostandzhyan <jin@mediatomb.org>,
                       Leonhard Wimmer <leo@mediatomb.org>
    
    MediaTomb is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
    
    MediaTomb is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    along with MediaTomb; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
    
    $Id$
*/

/// \file stringinput.h

#ifndef __MXML_STRINGINPUT_H__
#define __MXML_STRINGINPUT_H__

#include "zmmf/zmmf.h"
#include "input.h"

namespace mxml
{

class StringInput : public Input
{
protected:
    zmm::String str;
    char *ptr;
public:
    StringInput(zmm::String str);
	virtual int readChar();
};

}

#endif // __MXML_STRINGINPUT_H__

