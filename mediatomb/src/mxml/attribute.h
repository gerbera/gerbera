/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    attribute.h - this file is part of MediaTomb.
    
    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>
    
    Copyright (C) 2006-2007 Gena Batyan <bgeradz@mediatomb.cc>,
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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
    
    $Id$
*/

/// \file attribute.h

#ifndef __MXML_ATTRIBUTE_H__
#define __MXML_ATTRIBUTE_H__

#include "zmmf/zmmf.h"

namespace mxml
{

class Attribute : public zmm::Object
{
public:
	zmm::String name;
	zmm::String value;
public:
	Attribute(zmm::String name);
	Attribute(zmm::String name, zmm::String value);
	void setValue(zmm::String value);
};

}

#endif // __MXML_ATTRIBUTE_H__

