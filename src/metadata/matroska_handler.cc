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
class FileIOCallback : public IOCallback {
private:
    std::FILE* m_file;

public:
    explicit FileIOCallback(const char* path)
#ifdef __linux__
        : m_file(std::fopen(path, "rbe"))
#else
        : file(std::fopen(path, "rb"))
#endif
    {
        if (!m_file) {
            throw_std_runtime_error("Could not fopen {}", path);
        }
    }

    ~FileIOCallback() override
    {
        close();
    }

    FileIOCallback(const FileIOCallback&) = delete;
    FileIOCallback& operator=(const FileIOCallback&) = delete;

    uint32 read(void* buffer, std::size_t size) override
    {
        assert(m_file);
        if (size == 0)
            return 0;
        return std::fread(buffer, 1, size, m_file);
    }

    void setFilePointer(int64_t offset, seek_mode mode = seek_beginning) override
    {
        assert(m_file);
        assert(mode == SEEK_CUR || mode == SEEK_END || mode == SEEK_SET);
        if (fseeko(m_file, offset, mode) != 0) {
            throw_std_runtime_error("fseek failed");
        }
    }

    std::size_t write(const void* pBuffer, std::size_t iSize) override
    {
        // not needed
        return 0;
    }

    uint64 getFilePointer() override
    {
        assert(m_file);
        return ftello(m_file);
    }

    void close() override
    {
        if (!m_file)
            return;
        if (std::fclose(m_file) != 0) {
            log_error("fclose failed");
        }
        m_file = nullptr;
    }
};

void MatroskaHandler::fillMetadata(const std::shared_ptr<CdsObject>& obj)
{
    auto item = std::dynamic_pointer_cast<CdsItem>(obj);
    if (!item)
        return;

    parseMKV(item, nullptr);
}

std::unique_ptr<IOHandler> MatroskaHandler::serveContent(const std::shared_ptr<CdsObject>& obj, int resNum)
{
    auto item = std::dynamic_pointer_cast<CdsItem>(obj);
    if (!item)
        return nullptr;

    std::unique_ptr<MemIOHandler> io_handler;
    parseMKV(item, &io_handler);

    return io_handler;
}

void MatroskaHandler::parseMKV(const std::shared_ptr<CdsItem>& item, std::unique_ptr<MemIOHandler>* pIoHandler) const
{
    auto ebml_file = FileIOCallback(item->getLocation().c_str());
    auto ebml_stream = EbmlStream(ebml_file);

    auto el_l0 = ebml_stream.FindNextID(LIBMATROSKA_NAMESPACE::KaxSegment::ClassInfos, ~0);
    while (el_l0) {
        int i_upper_level = 0;
        EbmlElement* el_l1;
        while ((el_l1 = ebml_stream.FindNextElement(el_l0->Generic().Context, i_upper_level, ~0, true))) {
            parseLevel1Element(item, ebml_stream, el_l1, pIoHandler);

            el_l1->SkipData(ebml_stream, el_l1->Generic().Context);
            delete el_l1;
        } // while elementLevel1

        el_l0->SkipData(ebml_stream, LIBMATROSKA_NAMESPACE::KaxSegment_Context);
        delete el_l0;

        el_l0 = ebml_stream.FindNextElement(LIBMATROSKA_NAMESPACE::KaxSegment_Context, i_upper_level, ~0, true);
    } // while elementLevel0

    ebml_file.close();
}

void MatroskaHandler::parseLevel1Element(const std::shared_ptr<CdsItem>& item, EbmlStream& ebmlStream, EbmlElement* elL1, std::unique_ptr<MemIOHandler>* pIoHandler) const
{
    // Looking at just at EbmlId is not reliable since it can be a dummy element.
    if (!elL1->IsMaster())
        return;
    auto master = dynamic_cast<EbmlMaster*>(elL1);
    if (!master) {
        log_debug("dynamic_cast unexpectedly returned nullptr, seems to be broken");
        return;
    }
    if (EbmlId(*master) == LIBMATROSKA_NAMESPACE::KaxInfo::ClassInfos.GlobalId) {
        parseInfo(item, ebmlStream, master);
    } else if (EbmlId(*master) == LIBMATROSKA_NAMESPACE::KaxAttachments::ClassInfos.GlobalId) {
        parseAttachments(item, ebmlStream, master, pIoHandler);
    }
}

void MatroskaHandler::parseInfo(const std::shared_ptr<CdsItem>& item, EbmlStream& ebmlStream, EbmlMaster* info) const
{
    EbmlElement* dummy_el;
    int i_upper_level = 0;

    // master elements
    info->Read(ebmlStream, EBML_CONTEXT(info), i_upper_level, dummy_el, true);

    auto sc = StringConverter::i2i(config); // sure is sure

    for (auto&& el : *info) {
        if (EbmlId(*el) == LIBMATROSKA_NAMESPACE::KaxTitle::ClassInfos.GlobalId) {
            auto title_el = dynamic_cast<LIBMATROSKA_NAMESPACE::KaxTitle*>(el);
            if (!title_el) {
                log_error("Malformed MKV file; KaxTitle cast failed!");
                continue;
            }
            std::string title(UTFstring(*title_el).GetUTF8());
            log_debug("KaxTitle = {}", title);
            item->addMetaData(M_TITLE, sc->convert(title));
        } else if (EbmlId(*el) == LIBMATROSKA_NAMESPACE::KaxDateUTC::ClassInfos.GlobalId) {
            auto date_el = dynamic_cast<LIBMATROSKA_NAMESPACE::KaxDateUTC*>(el);
            if (!date_el) {
                log_error("Malformed MKV file; KaxDateUTC cast failed!");
                continue;
            }
            auto f_date = fmt::format("{:%Y-%m-%d}", fmt::gmtime(date_el->GetEpochDate()));
            if (!f_date.empty()) {
                log_debug("KaxDateUTC = {}", f_date);
                item->addMetaData(M_DATE, sc->convert(f_date));
            }
        }
    }
}

void MatroskaHandler::parseAttachments(const std::shared_ptr<CdsItem>& item, EbmlStream& ebmlStream, EbmlMaster* attachments, std::unique_ptr<MemIOHandler>* pIoHandler) const
{
    EbmlElement* dummy_el;
    int i_upper_level = 0;

    attachments->Read(ebmlStream, EBML_CONTEXT(attachments), i_upper_level, dummy_el, true);

    auto attachedFile = FindChild<LIBMATROSKA_NAMESPACE::KaxAttached>(*attachments);
    while (attachedFile && (attachedFile->GetSize() > 0)) {
        std::string fileName(UTFstring(GetChild<LIBMATROSKA_NAMESPACE::KaxFileName>(*attachedFile)).GetUTF8());
        log_debug("KaxFileName = {}", fileName);

        if (startswith(fileName, "cover")) {
            const auto& fileData = GetChild<LIBMATROSKA_NAMESPACE::KaxFileData>(*attachedFile);
            log_debug("KaxFileData (size={})", fileData.GetSize());

            if (pIoHandler) {
                // serveContent
                *pIoHandler = std::make_unique<MemIOHandler>(fileData.GetBuffer(), fileData.GetSize());
            } else {
                // fillMetadata
                std::string art_mimetype = getContentTypeFromByteVector(fileData);
                if (!art_mimetype.empty())
                    addArtworkResource(item, art_mimetype);
            }
        }

        attachedFile = &GetNextChild<LIBMATROSKA_NAMESPACE::KaxAttached>(*attachments, *attachedFile);
    }
}

std::string MatroskaHandler::getContentTypeFromByteVector(const LIBMATROSKA_NAMESPACE::KaxFileData& data) const
{
#ifdef HAVE_MAGIC
    auto art_mimetype = mime->bufferToMimeType(data.GetBuffer(), data.GetSize());
    return art_mimetype.empty() ? MIMETYPE_DEFAULT : art_mimetype;
#else
    return MIMETYPE_DEFAULT;
#endif
}

void MatroskaHandler::addArtworkResource(const std::shared_ptr<CdsItem>& item, const std::string& artMimetype)
{
    // if we could not determine the mimetype, then there is no
    // point to add the resource - it's probably garbage
    log_debug("Found artwork of type {} in file {}", artMimetype, item->getLocation().c_str());

    if (artMimetype != MIMETYPE_DEFAULT) {
        auto resource = std::make_shared<CdsResource>(CH_MATROSKA);
        resource->addAttribute(R_PROTOCOLINFO, renderProtocolInfo(artMimetype));
        resource->addParameter(RESOURCE_CONTENT_TYPE, ID3_ALBUM_ART);
        item->addResource(resource);
    }
}

#endif // HAVE_MATROSKA
