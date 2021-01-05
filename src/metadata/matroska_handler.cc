/*GRB*

    Gerbera - https://gerbera.io/
    
    matroska_handler.cc - this file is part of Gerbera.
    
    Copyright (C) 2019-2021 Gerbera Contributors
    
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

/// \file matroska_handler.cc

#ifdef HAVE_MATROSKA
#include "matroska_handler.h" // API

#include <iostream>
#include <utility>
#include <vector>

#include <ebml/EbmlHead.h>
#include <ebml/EbmlStream.h>
#include <ebml/EbmlSubHead.h>
#include <ebml/IOCallback.h>

#include <matroska/KaxAttached.h>
#include <matroska/KaxAttachments.h>
#include <matroska/KaxCluster.h>
#include <matroska/KaxContexts.h>
#include <matroska/KaxSeekHead.h>
#include <matroska/KaxSegment.h>

#include "cds_objects.h"
#include "config/config_manager.h"
#include "iohandler/mem_io_handler.h"
#include "util/mime.h"
#include "util/string_converter.h"
#include "util/tools.h"

using namespace LIBEBML_NAMESPACE;
using namespace LIBMATROSKA_NAMESPACE;

// file managment
class file_io_callback : public IOCallback {
private:
    FILE* file;

public:
    explicit file_io_callback(const char* path)
    {
#ifdef __linux__
        file = ::fopen(path, "rbe");
#else
        file = ::fopen(path, "rb");
#endif
        if (file == nullptr) {
            throw_std_runtime_error(std::string("Could not fopen ") + path);
        }
    }

    ~file_io_callback() override
    {
        close();
    }

    file_io_callback(const file_io_callback&) = delete;
    file_io_callback& operator=(const file_io_callback&& rhs) = delete;
    file_io_callback& operator=(const file_io_callback&) = delete;
    file_io_callback(const file_io_callback&& rhs) = delete;

    uint32 read(void* buffer, size_t size) override
    {
        assert(file != nullptr);
        if (size == 0)
            return 0;
        return fread(buffer, 1, size, file);
    }

    void setFilePointer(int64_t offset, seek_mode mode = seek_beginning) override
    {
        assert(file != nullptr);
        assert(mode == SEEK_CUR || mode == SEEK_END || mode == SEEK_SET);
        if (fseeko(file, offset, mode) != 0) {
            throw_std_runtime_error("fseek failed");
        }
    }

    size_t write(const void* p_buffer, size_t i_size) override
    {
        // not needed
        return 0;
    }

    uint64 getFilePointer() override
    {
        assert(file != nullptr);
        return ftello(file);
    }

    void close() override
    {
        if (file == nullptr)
            return;
        if (fclose(file) != 0) {
            throw_std_runtime_error("fclose failed");
        }
        file = nullptr;
    }
};

MatroskaHandler::MatroskaHandler(std::shared_ptr<Config> config, std::shared_ptr<Mime> mime)
    : MetadataHandler(std::move(config), std::move(mime))
{
}

void MatroskaHandler::fillMetadata(std::shared_ptr<CdsItem> item)
{
    parseMKV(item, nullptr);
}

std::unique_ptr<IOHandler> MatroskaHandler::serveContent(std::shared_ptr<CdsItem> item, int resNum)
{
    MemIOHandler* io_handler;
    parseMKV(item, &io_handler);

    std::unique_ptr<IOHandler> h;
    h.reset(io_handler);
    return h;
}

void MatroskaHandler::parseMKV(const std::shared_ptr<CdsItem>& item, MemIOHandler** p_io_handler) const
{
    file_io_callback ebml_file(item->getLocation().c_str());
    EbmlStream ebml_stream(ebml_file);

    EbmlElement* el_l0 = ebml_stream.FindNextID(KaxSegment::ClassInfos, ~0);
    while (el_l0 != nullptr) {
        int i_upper_level = 0;
        EbmlElement* el_l1 = ebml_stream.FindNextElement(el_l0->Generic().Context, i_upper_level, ~0, true);
        while (el_l1 != nullptr) {
            parseLevel1Element(item, ebml_stream, el_l1, p_io_handler);

            el_l1->SkipData(ebml_stream, el_l1->Generic().Context);
            delete el_l1;

            el_l1 = ebml_stream.FindNextElement(el_l0->Generic().Context, i_upper_level, ~0, true);
        } // while elementLevel1

        el_l0->SkipData(ebml_stream, KaxSegment_Context);
        delete el_l0;

        el_l0 = ebml_stream.FindNextElement(KaxSegment_Context, i_upper_level, ~0, true);
    } // while elementLevel0

    ebml_file.close();
}

void MatroskaHandler::parseLevel1Element(const std::shared_ptr<CdsItem>& item, EbmlStream& ebml_stream, EbmlElement* el_l1, MemIOHandler** p_io_handler) const
{
    // Looking at just at EbmlId is not reliable since it can be a dummy element.
    if (!el_l1->IsMaster())
        return;
    auto master = dynamic_cast<EbmlMaster*>(el_l1);
    if (!master) {
        log_debug("dynamic_cast unexpectedly returned nullpt, seems to be broken");
        return;
    }
    if (EbmlId(*master) == KaxInfo::ClassInfos.GlobalId) {
        parseInfo(item, ebml_stream, master);
    } else if (EbmlId(*master) == KaxAttachments::ClassInfos.GlobalId) {
        parseAttachments(item, ebml_stream, master, p_io_handler);
    }
}

void MatroskaHandler::parseInfo(const std::shared_ptr<CdsItem>& item, EbmlStream& ebml_stream, EbmlMaster* info) const
{
    EbmlElement* dummy_el;
    int i_upper_level = 0;

    // master elements
    info->Read(ebml_stream, EBML_CONTEXT(info), i_upper_level, dummy_el, true);

    auto sc = StringConverter::i2i(config); // sure is sure

    for (const auto& el : *info) {
        if (EbmlId(*el) == KaxTitle::ClassInfos.GlobalId) {
            auto title_el = dynamic_cast<KaxTitle*>(el);
            if (title_el == nullptr) {
                log_error("Malformed MKV file; KaxTitle cast failed!");
                continue;
            }
            std::string title(UTFstring(*title_el).GetUTF8());
            // printf("KaxTitle = %s\n", title.c_str());
            item->setMetadata(M_TITLE, sc->convert(title));
        } else if (EbmlId(*el) == KaxDateUTC::ClassInfos.GlobalId) {
            auto date_el = dynamic_cast<KaxDateUTC*>(el);
            if (date_el == nullptr) {
                log_error("Malformed MKV file; KaxDateUTC cast failed!");
                continue;
            }
            KaxDateUTC& date = *date_el;
            time_t i_date;
            struct tm tmres;
            std::array<char, 25> buffer;
            i_date = date.GetEpochDate();
            if (gmtime_r(&i_date, &tmres) && strftime(buffer.data(), buffer.size(), "%Y-%m-%d", &tmres)) {
                // printf("KaxDateUTC = %s\n", buffer);
                item->setMetadata(M_DATE, sc->convert(buffer.data()));
            }
        }
    }
}

void MatroskaHandler::parseAttachments(const std::shared_ptr<CdsItem>& item, EbmlStream& ebml_stream, EbmlMaster* attachments, MemIOHandler** p_io_handler) const
{
    EbmlElement* dummy_el;
    int i_upper_level = 0;

    attachments->Read(ebml_stream, EBML_CONTEXT(attachments), i_upper_level, dummy_el, true);

    auto attachedFile = FindChild<KaxAttached>(*attachments);
    while (attachedFile && (attachedFile->GetSize() > 0)) {
        std::string fileName(UTFstring(GetChild<KaxFileName>(*attachedFile)).GetUTF8());
        // printf("KaxFileName = %s\n", fileName.c_str());

        bool isCoverArt = false;
        if (startswith(fileName, "cover")) {
            isCoverArt = true;
        }

        if (isCoverArt) {
            auto& fileData = GetChild<KaxFileData>(*attachedFile);
            // printf("KaxFileData (size=%ld)\n", fileData.GetSize());

            if (p_io_handler != nullptr) {
                // serveContent
                *p_io_handler = new MemIOHandler(fileData.GetBuffer(), fileData.GetSize());
            } else {
                // fillMetadata
                std::string art_mimetype = getContentTypeFromByteVector(&fileData);
                if (!art_mimetype.empty())
                    addArtworkResource(item, art_mimetype);
            }
        }

        attachedFile = &GetNextChild<KaxAttached>(*attachments, *attachedFile);
    }
}

std::string MatroskaHandler::getContentTypeFromByteVector(const KaxFileData* data) const
{
    std::string art_mimetype = MIMETYPE_DEFAULT;
#ifdef HAVE_MAGIC
    art_mimetype = mime->bufferToMimeType(data->GetBuffer(), data->GetSize());
    if (art_mimetype.empty()) {
        return MIMETYPE_DEFAULT;
    }
#endif
    return art_mimetype;
}

void MatroskaHandler::addArtworkResource(const std::shared_ptr<CdsItem>& item, const std::string& art_mimetype)
{
    // if we could not determine the mimetype, then there is no
    // point to add the resource - it's probably garbage
    log_debug("Found artwork of type {} in file {}", art_mimetype.c_str(), item->getLocation().c_str());

    if (art_mimetype != MIMETYPE_DEFAULT) {
        auto resource = std::make_shared<CdsResource>(CH_MATROSKA);
        resource->addAttribute(R_PROTOCOLINFO, renderProtocolInfo(art_mimetype));
        resource->addParameter(RESOURCE_CONTENT_TYPE, ID3_ALBUM_ART);
        item->addResource(resource);
    }
}

#endif // HAVE_MATROSKA
