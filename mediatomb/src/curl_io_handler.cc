/*MT*/

/// \file curl_io_handler.cc

#ifdef HAVE_CONFIG_H
    #include "autoconfig.h"
#endif

#ifdef HAVE_CURL

#include "tools.h"
#include "curl_io_handler.h"

using namespace zmm;

CurlIOHandler::CurlIOHandler(String URL, CURL *curl_handle, size_t bufSize, size_t initialFillSize) : IOHandlerBufferHelper(bufSize, initialFillSize)
{
    if (! string_ok(URL))
        throw _Exception(_("URL has not been set correctly"));
    if (bufSize < CURL_MAX_WRITE_SIZE)
        throw _Exception(_("bufSize must be at least CURL_MAX_WRITE_SIZE(")+CURL_MAX_WRITE_SIZE+')');
    
    this->URL = URL;
    this->external_curl_handle = (curl_handle != NULL);
    this->curl_handle = curl_handle;
    //bytesCurl = 0;
    signalAfterEveryRead = true;
}

void CurlIOHandler::open(IN enum UpnpOpenFileMode mode)
{
    
    if (curl_handle == NULL)
    {
        curl_handle = curl_easy_init();
        if (curl_handle == NULL)
            throw _Exception(_("failed to init curl"));
    }
    else
        curl_easy_reset(curl_handle);
    
    IOHandlerBufferHelper::open(mode);
}

void CurlIOHandler::close()
{
    IOHandlerBufferHelper::close();
    
    if (external_curl_handle && curl_handle != NULL)
        curl_easy_cleanup(curl_handle);
}

void CurlIOHandler::threadProc()
{
    CURLcode res;
    assert(curl_handle != NULL);
    assert(string_ok(URL));
    
    //char error_buffer[CURL_ERROR_SIZE] = {'\0'};
    //curl_easy_setopt(curl_handle, CURLOPT_ERRORBUFFER, error_buffer);
    
    curl_easy_setopt(curl_handle, CURLOPT_URL, URL.c_str());
    curl_easy_setopt(curl_handle, CURLOPT_NOSIGNAL, 1);
    //curl_easy_setopt(curl_handle, CURLOPT_CONNECTTIMEOUT,
    
    //proxy..
    
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, CurlIOHandler::curlCallback);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)this);
    
    res = curl_easy_perform(curl_handle);
    if (res != CURLE_OK)
        readError = true;
    else
        eof = true;
    
    cond->signal();
}



size_t CurlIOHandler::curlCallback(void *ptr, size_t size, size_t nmemb, void *data)
{
    CurlIOHandler * ego = (CurlIOHandler *) data;
    size_t wantWrite = size * nmemb;
    
    assert(wantWrite <= ego->bufSize);
    
    //log_debug("URL: %s; size: %d; nmemb: %d; wantWrite: %d\n", ego->URL.c_str(), size, nmemb, wantWrite);
    
    AUTOLOCK(ego->mutex);
    
    bool first = true;
    
    int bufFree = 0;
    do
    {
        if (! first)
        {
            ego->cond->wait();
        }
        else
            first = false;
        
        if (ego->threadShutdown)
            return 0;
        
        if (ego->empty)
        {
            ego->a = ego->b = 0;
            bufFree = ego->bufSize;
        }
        else
        {
            bufFree = ego->a - ego->b;
            if (bufFree < 0)
                bufFree += ego->bufSize;
        }
    }
    while (bufFree < wantWrite);
    
    
    size_t maxWrite = (ego->empty ? ego->bufSize : (ego->a < ego->b ? ego->bufSize - ego->b : ego->a - ego->b));
    size_t write1 = (wantWrite > maxWrite ? maxWrite : wantWrite);
    size_t write2 = (write1 < wantWrite ? wantWrite - write1 : 0);
    
    size_t bLocal = ego->b;
    
    AUTOUNLOCK();
    
    memcpy(ego->buffer + bLocal, ptr, write1);
    if (write2)
        memcpy(ego->buffer, (char *)ptr + maxWrite, write2);
    
    AUTORELOCK();
    
    ego->b += wantWrite;
    if (ego->b >= ego->bufSize)
        ego->b -= ego->bufSize;
    if (ego->empty)
    {
        ego->empty = false;
        ego->cond->signal();
    }
    if (ego->waitForInitialFillSize)
    {
        int currentFillSize = ego->b - ego->a;
        if (currentFillSize <= 0)
            currentFillSize += ego->bufSize;
        if (currentFillSize >= ego->initialFillSize)
        {
            log_debug("buffer: initial fillsize reached\n");
            ego->waitForInitialFillSize = false;
            ego->cond->signal();
        }
    }
    
    //ego->bytesCurl += wantWrite;
    return wantWrite;
}

#endif//HAVE_CURL
