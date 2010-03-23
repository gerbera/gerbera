/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    comment.h - this file is part of MediaTomb.
    
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

/// \file comment.h

#ifndef __MXML_COMMENT_H__
#define __MXML_COMMENT_H__

#include "zmmf/zmmf.h"

#include "mxml.h"

#include "node.h"

namespace mxml
{

class Comment : public Node
{
protected:
    zmm::String name;
    zmm::String text;
    zmm::Ref<zmm::Array<Attribute> > attributes;
    bool indentWithLFbefore;

public:
    Comment(zmm::String text, bool indentWithLFbefore = false);
    inline zmm::String getText() { return text; }
    inline void setText(zmm::String text) { this->text = text; }
    inline bool getIndentWithLFbefore() { return indentWithLFbefore; }
    inline void setIndentWithLFbefore(bool indentWithLFbefore) { this->indentWithLFbefore = indentWithLFbefore; }

protected:
    virtual void print_internal(zmm::Ref<zmm::StringBuffer> buf, int indent);
};

} // namespace

#endif // __MXML_COMMENT_H__
