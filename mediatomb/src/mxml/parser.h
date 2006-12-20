/*MT*
    
    MediaTomb - http://www.mediatomb.org/
    
    parser.h - this file is part of MediaTomb.
    
    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.org>,
                       Sergey Bostandzhyan <jin@mediatomb.org>
    Copyright (C) 2006 Gena Batyan <bgeradz@mediatomb.org>,
                       Sergey Bostandzhyan <jin@mediatomb.org>,
                       Leonhard Wimmer <leo@mediatomb.org>
    
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

/// \file parser.h

#ifndef __MXML_PARSER_H__
#define __MXML_PARSER_H__

#include "zmmf/zmmf.h"

namespace mxml
{

class Element;
class Context;
class Input;
    
class Parser : public zmm::Object
{
public:
	Parser();
	zmm::Ref<Element> parseFile(zmm::String);
    zmm::Ref<Element> parseString(zmm::String);
protected:
	zmm::Ref<Element> parse(zmm::Ref<Context> ctx, zmm::Ref<Input> input,
                            zmm::String parentTag, int state);
};

}

#endif // __MXML_PARSER_H__

