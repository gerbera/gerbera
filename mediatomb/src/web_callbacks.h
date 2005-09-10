/*  web_callbacks.h - this file is part of MediaTomb.
                                                                                
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

/// \file web_callbacks.h
#ifndef WEB_CALLBACKS_H
#define WEB_CALLBACKS_H

/// \brief Registers callback functions for the internal web server.
///
/// This function registers callbacks for the internal web server.
/// The callback functions are:
/// \b web_get_info Query information on a file.
/// \b web_open Open a file.
/// \b web_read Sequentially read from a file.
/// \b web_write Sequentially write to a file (not supported).
/// \b web_seek Perform a seek on a file.
/// \b web_close Close file.
/// 
/// \return UPNP_E_SUCCESS Callbacks registered successfully, else eror code.
int register_web_callbacks();

#endif
