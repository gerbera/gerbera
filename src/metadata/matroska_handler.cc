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

#include <fmt/chrono.h>
#include <iostream>
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

// file managment
class file_io_callback : public IOCallback {
private:
    FILE* file;

public:
    explicit file_io_callback(const char* path)
#ifdef __linux__
        : file(::fopen(path, "rbe"))
#else
        : file(fopen(path, "rb"))
#endif
    {
        if (file == nullptr) {
            throw_std_runtime_error("Could not fopen {}", path);
        }
    }

    ~file_io_callback() override
    {
        close();
    }

    file_io_callback(const file_io_callback&) = delete;
    file_io_callback& operator=(const file_io_callback&) = delete;

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

MatroskaHandler::MatroskaHandler(const std::shared_ptr<Context>& context)
    : MetadataHandler(context)
{
}

void MatroskaHandler::fillMetadata(std::shared_ptr<CdsObject> obj)
{
    auto item = std::dynamic_pointer_cast<CdsItem>(obj);
    if (item == nullptr)
        return;

    parseMKV(item, nullptr);
}

std::unique_ptr<IOHandler> MatroskaHandler::serveContent(std::shared_ptr<CdsObject> obj, int resNum)
{
    auto item = std::dynamic_pointer_cast<CdsItem>(obj);
    if (item == nullptr)
        return nullptr;

    std::unique_ptr<MemIOHandler> io_handler;
    parseMKV(item, &io_handler);

    return io_handler;
}

void MatroskaHandler::parseMKV(const std::shared_ptr<CdsItem>& item, std::unique_ptr<MemIOHandler>* p_io_handler) const
{
    file_io_callback ebml_file(item->getLocation().c_str());
    EbmlStream ebml_stream(ebml_file);

    auto el_l0 = ebml_stream.FindNextID(LIBMATROSKA_NAMESPACE::KaxSegment::ClassInfos, ~0);
    while (el_l0 != nullptr) {
        int i_upper_level = 0;
        auto el_l1 = ebml_stream.FindNextElement(el_l0->Generic().Context, i_upper_level, ~0, true);
        while (el_l1 != nullptr) {
            parseLevel1Element(item, ebml_stream, el_l1, p_io_handler);

            el_l1->SkipData(ebml_stream, el_l1->Generic().Context);
            delete el_l1;

            el_l1 = ebml_stream.FindNextElement(el_l0->Generic().Context, i_upper_level, ~0, true);
        } // while elementLevel1

        el_l0->SkipData(ebml_stream, LIBMATROSKA_NAMESPACE::KaxSegment_Context);
        delete el_l0;

        el_l0 = ebml_stream.FindNextElement(LIBMATROSKA_NAMESPACE::KaxSegment_Context, i_upper_level, ~0, true);
    } // while elementLevel0

    ebml_file.close();
}

void MatroskaHandler::parseLevel1Element(const std::shared_ptr<CdsItem>& item, EbmlStream& ebml_stream, EbmlElement* el_l1, std::unique_ptr<MemIOHandler>* p_io_handler) const
{
    // Looking at just at EbmlId is not reliable since it can be a dummy element.
    if (!el_l1->IsMaster())
        return;
    auto master = dynamic_cast<EbmlMaster*>(el_l1);
    if (master == nullptr) {
        log_debug("dynamic_cast unexpectedly returned nullptr, seems to be broken");
        return;
    }
    if (EbmlId(*master) == LIBMATROSKA_NAMESPACE::KaxInfo::ClassInfos.GlobalId) {
        parseInfo(item, ebml_stream, master);
    } else if (EbmlId(*master) == LIBMATROSKA_NAMESPACE::KaxAttachments::ClassInfos.GlobalId) {
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

    for (auto&& el : *info) {
        if (EbmlId(*el) == LIBMATROSKA_NAMESPACE::KaxTitle::ClassInfos.GlobalId) {
            auto title_el = dynamic_cast<LIBMATROSKA_NAMESPACE::KaxTitle*>(el);
            if (title_el == nullptr) {
                log_error("Malformed MKV file; KaxTitle cast failed!");
                continue;
            }
            std::string title(UTFstring(*title_el).GetUTF8());
            log_debug("KaxTitle = {}", title);
            item->setMetadata(M_TITLE, sc->convert(title));
        } else if (EbmlId(*el) == LIBMATROSKA_NAMESPACE::KaxDateUTC::ClassInfos.GlobalId) {
            auto date_el = dynamic_cast<LIBMATROSKA_NAMESPACE::KaxDateUTC*>(el);
            if (date_el == nullptr) {
                log_error("Malformed MKV file; KaxDateUTC cast failed!");
                continue;
            }
            auto f_date = fmt::format("{:%Y-%m-%d}", fmt::gmtime(date_el->GetEpochDate()));
            if (!f_date.empty()) {
                log_debug("KaxDateUTC = {}", f_date);
                item->setMetadata(M_DATE, sc->convert(f_date));
            }
        }
    }
}

void MatroskaHandler::parseAttachments(const std::shared_ptr<CdsItem>& item, EbmlStream& ebml_stream, EbmlMaster* attachments, std::unique_ptr<MemIOHandler>* p_io_handler) const
{
    EbmlElement* dummy_el;
    int i_upper_level = 0;

    attachments->Read(ebml_stream, EBML_CONTEXT(attachments), i_upper_level, dummy_el, true);

    auto attachedFile = FindChild<LIBMATROSKA_NAMESPACE::KaxAttached>(*attachments);
    while (attachedFile && (attachedFile->GetSize() > 0)) {
        std::string fileName(UTFstring(GetChild<LIBMATROSKA_NAMESPACE::KaxFileName>(*attachedFile)).GetUTF8());
        log_debug("KaxFileName = {}", fileName);

        if (startswith(fileName, "cover")) {
            auto&& fileData = GetChild<LIBMATROSKA_NAMESPACE::KaxFileData>(*attachedFile);
            log_debug("KaxFileData (size={})", fileData.GetSize());

            if (p_io_handler != nullptr) {
                // serveContent
                *p_io_handler = std::make_unique<MemIOHandler>(fileData.GetBuffer(), fileData.GetSize());
            } else {
                // fillMetadata
                std::string art_mimetype = getContentTypeFromByteVector(&fileData);
                if (!art_mimetype.empty())
                    addArtworkResource(item, art_mimetype);
            }
        }

        attachedFile = &GetNextChild<LIBMATROSKA_NAMESPACE::KaxAttached>(*attachments, *attachedFile);
    }
}

std::string MatroskaHandler::getContentTypeFromByteVector(const LIBMATROSKA_NAMESPACE::KaxFileData* data) const
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
