
/*  scripting.cc - this file is part of MediaTomb.
                                                                                
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

#include "autoconfig.h"

#ifdef HAVE_JS

#include "runtime.h"

using namespace zmm;

static Ref<Runtime> instance;

Runtime::Runtime() : Object()
{
    mutex = Ref<Mutex>(new Mutex(true)); // need recursive mutex

    /* initialize the JS run time, and return result in rt */
    rt = JS_NewRuntime(8L * 1024L * 1024L);
    if (!rt)
        throw Exception(_("Scripting: could not initialize js runtime"));
}
Runtime::~Runtime()
{
	if (rt)
		JS_DestroyRuntime(rt);
}

JSRuntime *Runtime::getRT()
{
    return rt;
}

Ref<Runtime> Runtime::getInstance()
{
    if (instance == nil)
        instance = Ref<Runtime>(new Runtime());
    return instance;
}

#endif // HAVE_JS

