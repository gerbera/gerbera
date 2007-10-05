/*MT*/

/// \file curl_io_handler.h

#ifdef HAVE_CURL

#ifndef __CURL_IO_HANDLER_H__
#define __CURL_IO_HANDLER_H__

#include <curl/curl.h>
#include "common.h"
#include "upnp.h"
#include "io_handler_buffer_helper.h"

class CurlIOHandler : public IOHandlerBufferHelper
{
public:
    
    CurlIOHandler(zmm::String URL, CURL *curl_handle, size_t bufSize, size_t initialFillSize);
    
    virtual void open(enum UpnpOpenFileMode mode);
    virtual void close();
    
private:
    CURL *curl_handle;
    bool external_curl_handle;
    zmm::String URL;
    //off_t bytesCurl;
    
    static size_t curlCallback(void *ptr, size_t size, size_t nmemb, void *stream);
    virtual void threadProc();
};

#endif // __CURL_IO_HANDLER_H__

#endif//HAVE_CURL
