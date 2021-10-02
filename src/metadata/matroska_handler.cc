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
    std::FILE* mediaFile;

public:
    explicit FileIOCallback(const char* path)
#ifdef __linux__
        : mediaFile(std::fopen(path, "rbe"))
#else
        : mediaFile(std::fopen(path, "rb"))
#endif
    {
        if (!mediaFile) {
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
        assert(mediaFile);
        if (size == 0)
            return 0;
        return std::fread(buffer, 1, size, mediaFile);
    }

    void setFilePointer(int64_t offset, seek_mode mode = seek_beginning) override
    {
        assert(mediaFile);
        assert(mode == SEEK_CUR || mode == SEEK_END || mode == SEEK_SET);
        if (fseeko(mediaFile, offset, mode) != 0) {
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
        assert(mediaFile);
        return ftello(mediaFile);
    }

    void close() override
    {
        if (!mediaFile)
            return;
        if (std::fclose(mediaFile) != 0) {
            log_error("fclose failed");
        }
        mediaFile = nullptr;
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

    std::unique_ptr<MemIOHandler> ioHandler;
    parseMKV(item, &ioHandler);

    return ioHandler;
}

void MatroskaHandler::parseMKV(const std::shared_ptr<CdsItem>& item, std::unique_ptr<MemIOHandler>* pIoHandler) const
{
    auto ebmlFile = FileIOCallback(item->getLocation().c_str());
    auto ebmlStream = EbmlStream(ebmlFile);

    auto elL0 = ebmlStream.FindNextID(LIBMATROSKA_NAMESPACE::KaxSegment::ClassInfos, ~0);
    while (elL0) {
        int iUpperLevel = 0;
        EbmlElement* elL1;
        while ((elL1 = ebmlStream.FindNextElement(elL0->Generic().Context, iUpperLevel, ~0, true))) {
            parseLevel1Element(item, ebmlStream, elL1, pIoHandler);

            elL1->SkipData(ebmlStream, elL1->Generic().Context);
            delete elL1;
        } // while elementLevel1

        elL0->SkipData(ebmlStream, LIBMATROSKA_NAMESPACE::KaxSegment_Context);
        delete elL0;

        elL0 = ebmlStream.FindNextElement(LIBMATROSKA_NAMESPACE::KaxSegment_Context, iUpperLevel, ~0, true);
    } // while elementLevel0

    ebmlFile.close();
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
    EbmlElement* dummyEl;
    int iUpperLevel = 0;

    // master elements
    info->Read(ebmlStream, EBML_CONTEXT(info), iUpperLevel, dummyEl, true);

    auto sc = StringConverter::i2i(config); // sure is sure

    for (auto&& el : *info) {
        if (EbmlId(*el) == LIBMATROSKA_NAMESPACE::KaxTitle::ClassInfos.GlobalId) {
            auto titleEl = dynamic_cast<LIBMATROSKA_NAMESPACE::KaxTitle*>(el);
            if (!titleEl) {
                log_error("Malformed MKV file; KaxTitle cast failed!");
                continue;
            }
            std::string title(UTFstring(*titleEl).GetUTF8());
            log_debug("KaxTitle = {}", title);
            item->addMetaData(M_TITLE, sc->convert(title));
        } else if (EbmlId(*el) == LIBMATROSKA_NAMESPACE::KaxDateUTC::ClassInfos.GlobalId) {
            auto dateEl = dynamic_cast<LIBMATROSKA_NAMESPACE::KaxDateUTC*>(el);
            if (!dateEl) {
                log_error("Malformed MKV file; KaxDateUTC cast failed!");
                continue;
            }
            auto fDate = fmt::format("{:%Y-%m-%d}", fmt::gmtime(dateEl->GetEpochDate()));
            if (!fDate.empty()) {
                log_debug("KaxDateUTC = {}", fDate);
                item->addMetaData(M_DATE, sc->convert(fDate));
            }
        }
    }
}

void MatroskaHandler::parseAttachments(const std::shared_ptr<CdsItem>& item, EbmlStream& ebmlStream, EbmlMaster* attachments, std::unique_ptr<MemIOHandler>* pIoHandler) const
{
    EbmlElement* dummyEl;
    int iUpperLevel = 0;

    attachments->Read(ebmlStream, EBML_CONTEXT(attachments), iUpperLevel, dummyEl, true);

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
                std::string artMimetype = getContentTypeFromByteVector(fileData);
                if (!artMimetype.empty())
                    addArtworkResource(item, artMimetype);
            }
        }

        attachedFile = &GetNextChild<LIBMATROSKA_NAMESPACE::KaxAttached>(*attachments, *attachedFile);
    }
}

std::string MatroskaHandler::getContentTypeFromByteVector(const LIBMATROSKA_NAMESPACE::KaxFileData& data) const
{
#ifdef HAVE_MAGIC
    auto artMimetype = mime->bufferToMimeType(data.GetBuffer(), data.GetSize());
    return artMimetype.empty() ? MIMETYPE_DEFAULT : artMimetype;
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
