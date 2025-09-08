/*GRB*

    Gerbera - https://gerbera.io/

    wavpack_handler.h - this file is part of Gerbera.

    Copyright (C) 2022-2025 Gerbera Contributors

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

/// @file metadata/wavpack_handler.h
/// @brief Definition of the WavPackHandler class.

#ifndef __METADATA_WAVPACK_H__
#define __METADATA_WAVPACK_H__

#ifdef HAVE_WAVPACK

#include "metadata_handler.h"

#include <wavpack/wavpack.h>

// forward declarations
class CdsItem;

/// @brief This class is responsible for reading wavpack tags metadata
class WavPackHandler : public MediaMetadataHandler {
public:
    explicit WavPackHandler(const std::shared_ptr<Context>& context);
    ~WavPackHandler() override;
    WavPackHandler(const WavPackHandler&) = delete;
    WavPackHandler& operator=(const WavPackHandler&) = delete;

    bool isSupported(const std::string& contentType,
        bool isOggTheora,
        const std::string& mimeType,
        ObjectType mediaType) override;
    bool fillMetadata(const std::shared_ptr<CdsObject>& obj) override;
    std::unique_ptr<IOHandler> serveContent(
        const std::shared_ptr<CdsObject>& obj,
        const std::shared_ptr<CdsResource>& resource) override;

private:
    /// @brief read media attributes from stream
    static bool getAttributes(WavpackContext* context, const std::shared_ptr<CdsItem>& item);
    /// @brief get read meta data tags from file
    bool getTags(WavpackContext* context, const std::shared_ptr<CdsItem>& item);
    /// @brief get attachments like artwork
    bool getAttachments(WavpackContext* context, const std::shared_ptr<CdsItem>& item);
    /// @brief try to identify mime type from binary object
    std::string getMimeTypeFromByteVector(const char* data, int size) const;

    /// @brief reference to WavPack context
    WavpackContext* context = nullptr;
};

#endif // HAVE_WAVPACK

#endif // __METADATA_WAVPACK_H__
