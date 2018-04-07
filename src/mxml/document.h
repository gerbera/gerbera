/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    document.h - this file is part of MediaTomb.
    
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

/// \file document.h

#ifndef __MXML_DOCUMENT_H__
#define __MXML_DOCUMENT_H__

#include "zmm/zmmf.h"

#include "mxml.h"

namespace mxml
{
    
class Element;

class Document : public Node
{
protected:
    zmm::Ref<Element> root;
    
public:
    Document();
    
    zmm::Ref<Element> getRoot();
    void setRoot(zmm::Ref<Element> root);
    void appendChild(zmm::Ref<Node> child);
    
protected:
    void print_internal(std::ostringstream &buf, int indent) override;
};

} // namespace

#endif // __MXML_DOCUMENT_H__
