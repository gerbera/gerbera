/*GRB*

    Gerbera - https://gerbera.io/
    
    context.cc - this file is part of Gerbera.
    
    Copyright (C) 2021 Gerbera Contributors
    
    Gerbera is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.

    Gerbera is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Gerbera.  If not, see <http://www.gnu.org/licenses/>.

    $Id$
*/

/// \file layout.cc

#include "layout.h" // API

#include "content_manager.h"

Layout::Layout(std::shared_ptr<ContentManager> content)
    : config(content->getContext()->getConfig())
    , database(content->getContext()->getDatabase())
    , content(std::move(content))
{
}
