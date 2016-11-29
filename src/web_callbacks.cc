/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    web_callbacks.cc - this file is part of MediaTomb.
    
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

/// \file web_callbacks.cc
/// This handles the VirtualDirCallbacks that come from the web server.

#include <sys/types.h>

#include "common.h"
#include "tools.h"
#include "web_callbacks.h"
#include "server.h"
#include "request_handler.h"
#include "file_request_handler.h"
#ifdef HAVE_CURL
    #include "url_request_handler.h"
#endif
#include "web_request_handler.h"
#include "serve_request_handler.h"
#include "web/pages.h"

using namespace zmm;
using namespace mxml;

// FIXME Headers
static Ref<RequestHandler> create_request_handler(const char *filename)
{
    String path, parameters;

    String link = url_unescape((char *) filename);

    log_debug("Filename: %s, Path: %s\n", filename, path.c_str());
//    log_debug("create_handler: got url parameters: [%s]\n", parameters.c_str());
    
    RequestHandler *ret = NULL;
/*
    log_debug("Link is: %s, checking against: %s, starts with? %d\n", link.c_str(), 
                (String(SERVER_VIRTUAL_DIR) + "/" + CONTENT_UI_HANDLER).c_str(), 
                link.startsWith(String(SERVER_VIRTUAL_DIR) + "/" + CONTENT_UI_HANDLER));
    log_debug("Link is: %s, checking against: %s, starts with? %d\n", link.c_str(), 
                (String(SERVER_VIRTUAL_DIR) + "/" + CONTENT_MEDIA_HANDLER).c_str(), 
                link.startsWith(String(SERVER_VIRTUAL_DIR) + "/" + CONTENT_MEDIA_HANDLER));
  
    log_debug("Link is: %s, checking against: %s, starts with? %d\n", link.c_str(), 
                (String(SERVER_VIRTUAL_DIR) + "/" + CONTENT_SERVE_HANDLER).c_str(), 
                link.startsWith(String(SERVER_VIRTUAL_DIR) + "/" + CONTENT_SERVE_HANDLER));
*/  
    if (link.startsWith(_("/") + SERVER_VIRTUAL_DIR + "/" +
                        CONTENT_MEDIA_HANDLER))
    {
            ret = new FileRequestHandler();
    }
    else if (link.startsWith(_("/") + SERVER_VIRTUAL_DIR + "/" +
                             CONTENT_UI_HANDLER))
    {  
        RequestHandler::split_url(filename, URL_UI_PARAM_SEPARATOR, path, 
                parameters);
        Ref<Dictionary> dict(new Dictionary());
        dict->decode(parameters);
        
        String r_type = dict->get(_(URL_REQUEST_TYPE));
        if (r_type != nil)
        {
            ret = create_web_request_handler(r_type);
        }
        else
        {
            ret = create_web_request_handler(_("index"));
        }
    } 
    else if (link.startsWith(_("/") + SERVER_VIRTUAL_DIR + "/" +
                             CONTENT_SERVE_HANDLER))
    {
        if (string_ok(ConfigManager::getInstance()->getOption(CFG_SERVER_SERVEDIR)))
            ret = new ServeRequestHandler();
        else
            throw _Exception(_("Serving directories is not enabled in configuration"));
    }
    /// \todo add enable/disable curl to configure.ac, currently this is automatically triggered depending on youtube and external transcoding definitions
#if defined(HAVE_CURL)
    else if (link.startsWith(_("/") + SERVER_VIRTUAL_DIR + "/" +
                             CONTENT_ONLINE_HANDLER))
    {
        ret = new URLRequestHandler();
    }
#endif
#if defined(HAVE_LIBDVDREAD_DISABLED)
    else if (link.startsWith(_("/") + SERVER_VIRTUAL_DIR + "/" + 
                CONTENT_DVD_HANDLER))
    {
        ret = new DVDRequestHandler();
    }
#endif
    else
    {
        throw _Exception(_("no valid handler type in ") + filename);
    }
    return Ref<RequestHandler>(ret);
}

/// \brief Query information on a file.
/// \param filename Name of the file to query.
/// \param File_Info Pointer to the struction that stores information about
/// the file.
///
/// This function corresponds to get_info from the UpnpVirtualDirCallbacks 
/// structure. It is called by the web server to query information about a
/// file. To perform the operation an appropriate request handler is 
/// created, it then takes care of filling in the data.
///
/// \return 0 Success.
/// \return -1 Error.

// FIXME headers
static int web_get_info(IN const char *filename, OUT UpnpFileInfo *info)
{
    try
    {
        Ref<RequestHandler> reqHandler = create_request_handler(filename);
        reqHandler->get_info(filename, info);
    }
    catch (ServerShutdownException se)
    {
        return -1;
    }
    catch (SubtitlesNotFoundException sex)
    {
        log_info("%s\n", sex.getMessage().c_str());
        return -1;
    }
    catch (Exception e)
    {
        log_error("%s\n", e.getMessage().c_str());
        return -1;
    }

    return 0;
}

/// \brief Opens a file for the web server.
/// \param filename Name of the file to open (or actually the full URL)
/// \param mode in which the file will be opened (we only support UPNP_READ)
/// \param File_Info Pointer to the struction that stores information about
/// the file.
///
/// This function is called by the web server when it needs to open a file.
/// It first calls create_request_handler() which tries to identify the
/// request and returns us the appropriate IOHandler.
/// Note, that we return values here, because the SDK does not work with
/// exceptions.
///
/// \return UpnpWebFileHandle A valid file handle.
/// \return NULL Error.

// FIXME Headers
static UpnpWebFileHandle web_open(IN const char *filename,
                                  IN enum UpnpOpenFileMode mode)
{
    log_debug("web_open():");

    String link = url_unescape((char *) filename);

    //char *line = strstr((char *)headers, "TimeSeekRange.dlna.org: npt=");
    char *line = NULL;
    char *timeseek;
    if (line != NULL)
    {
         char *lineend = strstr(line, "\n");
         int chars = lineend - (line + 28);
         // limit to some value that makes sense to prevent allocating too much
         // memory due to some maliciously prepared headers
         if (chars < 1024)
         {
             timeseek = (char *)malloc(chars);
             if (timeseek)
             {
                 strncpy(timeseek, line + 28, chars);
                 log_debug("timeseek range found: %s\n",timeseek);
                 link = link + "/range/" + timeseek;
             }
         }
    }

    try
    {
        Ref<RequestHandler> reqHandler = create_request_handler(filename);
        Ref<IOHandler> ioHandler = reqHandler->open(link.c_str(), mode, nil);
        ioHandler->retain();
        return (UpnpWebFileHandle) ioHandler.getPtr();
    }
    catch (ServerShutdownException se)
    {
        return NULL;
    }
    catch (SubtitlesNotFoundException sex)
    {
        log_info("%s\n", sex.getMessage().c_str());
        return NULL;
    }
    catch (Exception ex)
    {
        log_error("%s\n", ex.getMessage().c_str());
        return NULL;
    }
}


/// \brief Reads a previously opened file sequentially.
/// \param f IOHandler that takes care of this request.
/// \param buf This buffer will be filled by our read functions.
/// \param length Number of bytes to read.
///
/// This function is called by the web server to perform a sequential
/// read from an open file. It calls the read function of the 
/// appropriate IOHandler, which copies \b length bytes from the file
/// or memory into the output buffer.
///
/// \return 0   EOF encountered.
/// \return -1  Error.
static int web_read(IN UpnpWebFileHandle f, OUT char *buf, 
                    IN size_t length)
{
    if (Server::getInstance()->getShutdownStatus())
        return -1;

    IOHandler *handler = (IOHandler *)f;
    return handler->read(buf, length);
}
                                                                                                                                                                         
/// \brief Writes to a previously opened file sequentially.
/// \param f Handle of the file.
/// \param buf This buffer will be filled by fwrite.
/// \param length Number of bytes to fwrite.
///
/// This function is called by the web server to perform a sequential
/// write to an open file. It copies \b length bytes into the file
/// from the buffer. It should return the actual number of bytes
/// written, in case of a write error this might be less than
/// \b length.
///
/// \return Actual number of bytes written.
///
/// \warning Currently this function is not supported.
static int web_write(IN UpnpWebFileHandle f, IN char *buf,
                      IN size_t length)
{
    return 0;
}
                                                                                                                                                                         
/// \brief Performs a seek on an open file.
/// \param f Handle of the file.
/// \param offset Number of bytes to move in the file. For seeking forwards
/// positive values are used, for seeking backwards - negative. \b Offset must
/// be positive if \b origin is set to \b SEEK_SET
/// \param whence The position to move relative to. SEEK_CUR to move relative
/// to current position, SEEK_END to move relative to the end of file,
/// SEEK_SET to specify an absolute offset.
///
/// This function is called by the web server to perform seek on an a file.
/// The seek operatoin itself is performed by the responsible IOHandler.
///
/// \return 0 On success, non-zero value on error.
static int web_seek(IN UpnpWebFileHandle f, IN off_t offset, IN int whence)
{
    try 
    {
        IOHandler *handler = (IOHandler *)f;
        handler->seek(offset, whence);
    }
    catch(Exception e)
    {
        log_error("web_seek(): Exception during seek: %s\n", e.getMessage().c_str());
        e.printStackTrace();
        return -1;
    }

    return 0;
}

/// \brief Closes a previously opened file.
/// \param f IOHandler for that file.
/// 
/// Same as fclose()
///
/// \return 0 On success, non-zero on error.
static int web_close( IN UpnpWebFileHandle f)
{

    Ref<IOHandler> handler((IOHandler *)f);
    handler->release();
    try
    {
        handler->close();
    }
    catch(Exception e)
    {
        log_error("web_seek(): Exception during seek: %s\n", e.getMessage().c_str());
        e.printStackTrace();
        return -1;
    }
    return 0;
}

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
int register_web_callbacks()
{
    int ret = 0;

    ret = UpnpVirtualDir_set_GetInfoCallback(web_get_info) == UPNP_E_SUCCESS
        && UpnpVirtualDir_set_OpenCallback(web_open) == UPNP_E_SUCCESS
        && UpnpVirtualDir_set_ReadCallback(web_read) == UPNP_E_SUCCESS
        && UpnpVirtualDir_set_WriteCallback(web_write) == UPNP_E_SUCCESS
        && UpnpVirtualDir_set_SeekCallback(web_seek) == UPNP_E_SUCCESS
        && UpnpVirtualDir_set_CloseCallback(web_close) == UPNP_E_SUCCESS;

    return ret ? UPNP_E_SUCCESS : UPNP_E_INVALID_PARAM;
}   
