/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    mxml.h - this file is part of MediaTomb.
    
    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>
    
    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
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

/// \file mxml.h

#ifndef __MXML_H__
#define __MXML_H__

#include "zmmf/zmmf.h"

namespace mxml
{
enum mxml_value_type
{
    mxml_string_type,
    mxml_bool_type,
    mxml_null_type,
    mxml_int_type
};
}

#include "attribute.h"
#include "context.h"
#include "node.h"
#include "element.h"
#include "document.h"
#include "xml_text.h"
#include "comment.h"
#include "parseexception.h"
#include "parser.h"
#include "xml_to_json.h"

#endif // __MXML_H__
