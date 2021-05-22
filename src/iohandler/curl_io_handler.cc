/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    curl_io_handler.cc - this file is part of MediaTomb.
    
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

/// \file curl_io_handler.cc

#ifdef HAVE_CURL
#include "curl_io_handler.h" // API

#include "config/config_manager.h"
#include "util/tools.h"

CurlIOHandler::CurlIOHandler(std::shared_ptr<Config> config, const std::string& URL, CURL* curl_handle, size_t bufSize, size_t initialFillSize)
    : IOHandlerBufferHelper(std::move(config), bufSize, initialFillSize)
{
    if (URL.empty())
        throw_std_runtime_error("URL has not been set correctly");
    if (bufSize < CURL_MAX_WRITE_SIZE)
        throw_std_runtime_error("bufSize must be at least CURL_MAX_WRITE_SIZE({})", CURL_MAX_WRITE_SIZE);

    this->URL = URL;
    this->external_curl_handle = (curl_handle != nullptr);
    this->curl_handle = curl_handle;
    //bytesCurl = 0;
    signalAfterEveryRead = true;

    // still todo:
    // * optimize seek if data already in buffer
    seekEnabled = true;
}

void CurlIOHandler::open(enum UpnpOpenFileMode mode)
{
    if (curl_handle == nullptr) {
        curl_handle = curl_easy_init();
        if (curl_handle == nullptr)
            throw_std_runtime_error("failed to init curl");
    } else
        curl_easy_reset(curl_handle);

    IOHandlerBufferHelper::open(mode);
}

void CurlIOHandler::close()
{
    IOHandlerBufferHelper::close();

    if (external_curl_handle && curl_handle != nullptr)
        curl_easy_cleanup(curl_handle);
}

void CurlIOHandler::threadProc()
{
    CURLcode res;
    assert(curl_handle != nullptr);
    assert(!URL.empty());

    //char error_buffer[CURL_ERROR_SIZE] = {'\0'};
    //curl_easy_setopt(curl_handle, CURLOPT_ERRORBUFFER, error_buffer);

    curl_easy_setopt(curl_handle, CURLOPT_URL, URL.c_str());
    curl_easy_setopt(curl_handle, CURLOPT_NOSIGNAL, 1);
    curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1);
    curl_easy_setopt(curl_handle, CURLOPT_MAXREDIRS, -1);

    bool logEnabled;
#ifdef TOMBDEBUG
    logEnabled = !ConfigManager::isDebugLogging();
#else
    logEnabled = ConfigManager::isDebugLogging();
#endif
    if (logEnabled)
        curl_easy_setopt(curl_handle, CURLOPT_VERBOSE, 1);

    //curl_easy_setopt(curl_handle, CURLOPT_CONNECTTIMEOUT,

    //proxy..

    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, CurlIOHandler::curlCallback);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, reinterpret_cast<void*>(this));

    auto lock = threadRunner->uniqueLock(std::defer_lock);
    do {
        lock.lock();
        if (doSeek) {
            log_debug("SEEK: {} {}", seekOffset, seekWhence);

            if (seekWhence == SEEK_SET) {
                posRead = seekOffset;
                curl_easy_setopt(curl_handle, CURLOPT_RESUME_FROM_LARGE, seekOffset);
            } else if (seekWhence == SEEK_CUR) {
                posRead += seekOffset;
                curl_easy_setopt(curl_handle, CURLOPT_RESUME_FROM_LARGE, posRead);
            } else {
                log_error("CurlIOHandler currently does not support SEEK_END");
                static_assert(true);
            }

            /// \todo should we do that?
            waitForInitialFillSize = (initialFillSize > 0);

            doSeek = false;
            threadRunner->notify();
        }
        lock.unlock();
        res = curl_easy_perform(curl_handle);
    } while (doSeek);

    if (res != CURLE_OK)
        readError = true;
    else
        eof = true;

    threadRunner->notify();
}

size_t CurlIOHandler::curlCallback(void* ptr, size_t size, size_t nmemb, void* data)
{
    auto ego = static_cast<CurlIOHandler*>(data);
    size_t wantWrite = size * nmemb;

    assert(wantWrite <= ego->bufSize);
    auto& threadRunner = ego->threadRunner;

    //log_debug("URL: {}; size: {}; nmemb: {}; wantWrite: {}", ego->URL.c_str(), size, nmemb, wantWrite);

    auto lock = threadRunner->uniqueLock();

    bool first = true;

    int bufFree = 0;
    do {
        if (ego->doSeek && !ego->empty && (ego->seekWhence == SEEK_SET || (ego->seekWhence == SEEK_CUR && ego->seekOffset > 0))) {
            auto currentFillSize = int(ego->b - ego->a);
            if (currentFillSize <= 0)
                currentFillSize += ego->bufSize;

            auto relSeek = int(ego->seekOffset);
            if (ego->seekWhence == SEEK_SET)
                relSeek -= ego->posRead;

            if (relSeek <= currentFillSize) { // we have everything we need in the buffer already
                ego->a += relSeek;
                ego->posRead += relSeek;
                if (ego->a >= ego->bufSize)
                    ego->a -= ego->bufSize;
                if (ego->a == ego->b) {
                    ego->empty = true;
                    ego->a = ego->b = 0;
                }
                /// \todo do we need to wait for initialFillSize again?

                ego->doSeek = false;
                threadRunner->notify();
            }
        }

        // note: seeking could be optimized some more (backward seeking)
        // but this should suffice for now

        if (ego->doSeek) { // seek not been processed yet
            ego->a = ego->b = 0;
            ego->empty = true;

            // terminate this request, because we need a new request
            // after the seek
            return 0;
        }

        if (!first) {
            threadRunner->wait(lock);
        } else
            first = false;

        if (ego->threadShutdown)
            return 0;

        if (ego->empty) {
            ego->a = ego->b = 0;
            bufFree = ego->bufSize;
        } else {
            bufFree = ego->a - ego->b;
            if (bufFree < 0)
                bufFree += ego->bufSize;
        }
    } while (size_t(bufFree) < wantWrite);

    size_t maxWrite = (ego->empty ? ego->bufSize : (ego->a < ego->b ? ego->bufSize - ego->b : ego->a - ego->b));
    size_t write1 = (wantWrite > maxWrite ? maxWrite : wantWrite);
    size_t write2 = (write1 < wantWrite ? wantWrite - write1 : 0);

    size_t bLocal = ego->b;

    lock.unlock();

    memcpy(ego->buffer + bLocal, ptr, write1);
    if (write2)
        memcpy(ego->buffer, static_cast<char*>(ptr) + maxWrite, write2);

    lock.lock();

    //ego->bytesCurl += wantWrite;
    ego->b += wantWrite;
    if (ego->b >= ego->bufSize)
        ego->b -= ego->bufSize;
    if (ego->empty) {
        ego->empty = false;
        threadRunner->notify();
    }
    if (ego->waitForInitialFillSize) {
        auto currentFillSize = int(ego->b - ego->a);
        if (currentFillSize <= 0)
            currentFillSize += ego->bufSize;
        if (size_t(currentFillSize) >= ego->initialFillSize) {
            log_debug("buffer: initial fillsize reached");
            ego->waitForInitialFillSize = false;
            threadRunner->notify();
        }
    }

    return wantWrite;
}

#endif //HAVE_CURL
