/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    node.h - this file is part of MediaTomb.
    
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

/// \file node.h

#ifndef __MXML_NODE_H__
#define __MXML_NODE_H__

#include "zmmf/zmmf.h"

#include "mxml.h"


namespace mxml
{

enum mxml_node_types
{
    mxml_node_all,
    mxml_node_element,
    mxml_node_text,
    mxml_node_comment,
    mxml_node_document
};

class Node : public zmm::Object
{
protected:
    zmm::Ref<zmm::Array<Node> > children;
    zmm::Ref<Context> context;
    enum mxml_node_types type;

public:
    enum mxml_node_types getType() { return type; }
    virtual zmm::String print();

    virtual void print_internal(zmm::Ref<zmm::StringBuffer> buf, int indent) = 0;
protected:
    static zmm::String escape(zmm::String str);
};


} // namespace

#endif // __MXML_NODE_H__
