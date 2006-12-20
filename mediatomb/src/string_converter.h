/*MT*
    
    MediaTomb - http://www.mediatomb.org/
    
    string_converter.h - this file is part of MediaTomb.
    
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

/// \file string_converter.h

#ifndef __STRING_CONVERTER_H__
#define __STRING_CONVERTER_H__

#include "common.h"
#include <iconv.h>

class StringConverter : public zmm::Object
{
protected:
    iconv_t cd;
    bool dirty;
public:
    StringConverter(zmm::String from, zmm::String to);
    virtual ~StringConverter();
    zmm::String convert(zmm::String str);
	static zmm::String validSubstring(zmm::String str, zmm::String encoding);
    
    static zmm::Ref<StringConverter> i2f();
    static zmm::Ref<StringConverter> f2i();
    static zmm::Ref<StringConverter> m2i();
};

#endif // __STRING_CONVERTER_H__
