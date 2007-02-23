/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    fallback_layout.h - this file is part of MediaTomb.
    
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
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
    
    $Id: scripting.h 1124 2007-02-17 21:49:15Z lww $
*/

/// \file fallback.h

#ifndef __FALLBACK_LAYOUT_H__
#define __FALLBACK_LAYOUT_H__

#include "layout.h"

class FallbackLayout : public Layout
{
public:
	FallbackLayout();
	virtual void init();
	virtual void processCdsObject(zmm::Ref<CdsObject> obj);	
};

#endif // __FALLBACK_LAYOUT_H__

