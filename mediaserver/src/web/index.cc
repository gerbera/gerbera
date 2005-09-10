/*  refresh.cc - this file is part of MediaTomb.
                                                                                
    Copyright (C) 2005 Gena Batyan <bgeradz@deadlock.dhs.org>,
                       Sergey Bostandzhyan <jin@deadlock.dhs.org>
                                                                                
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
*/

#include "common.h"
#include "pages.h"

using namespace zmm;
using namespace mxml;

web::index::index() : WebRequestHandler()
{
    pagename = "index";
}

void web::index::process()
{
    Ref<Element> suffer(new Element("suffer"));
    suffer->addAttribute("amount", "very much");
    
    Ref<Element> torture(new Element("torture"));
    torture->addAttribute("message", "and then you die");
    suffer->appendChild(torture);

    torture = Ref<Element>(new Element("torture"));
    torture->addAttribute("message", "pulverized");
    suffer->appendChild(torture);

    root->appendChild(suffer);
}

