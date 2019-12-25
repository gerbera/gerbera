/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    xpath.h - this file is part of MediaTomb.
    
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

/// \file xpath.h

#ifndef __XPATH_H__
#define __XPATH_H__

#include "mxml/mxml.h"
#include "zmm/zmmf.h"

class XPath : public zmm::Object {
protected:
    zmm::Ref<mxml::Element> context;

public:
    XPath(zmm::Ref<mxml::Element> context);
    std::string getText(std::string xpath);
    zmm::Ref<mxml::Element> getElement(std::string xpath);

    static std::string getPathPart(std::string xpath);
    static std::string getAxisPart(std::string xpath);
    static std::string getAxis(std::string axisPart);
    static std::string getSpec(std::string axisPart);

protected:
    zmm::Ref<mxml::Element> elementAtPath(std::string path);
};

#endif // __XPATH_H__
