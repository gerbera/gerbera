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
#include <vector>

#include <ebml/EbmlHead.h>
#include <ebml/EbmlStream.h>
#include <ebml/IOCallback.h>

#include <matroska/KaxAttachments.h>
#include <matroska/KaxContexts.h>
#include <matroska/KaxSeekHead.h>
#include <matroska/KaxSegment.h>

#include "cds_objects.h"
#include "iohandler/mem_io_handler.h"
#include "util/mime.h"
#include "util/string_converter.h"
#include "util/tools.h"

// file managment
class FileIOCallback : public IOCallback {
private:
    GrbFile file;
    std::FILE* mediaFile;

public:
    explicit FileIOCallback(const char* path)
        : file(path)
    {
        mediaFile = file.open("rb");
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

#define GRB_MATROSKA_TITLE 0x01
#define GRB_MATROSKA_DATE 0x02
#define GRB_MATROSKA_DURATION 0x04
#define GRB_MATROSKA_ARTWORK 0x08

#define GRB_MATROSKA_INFO (GRB_MATROSKA_TITLE | GRB_MATROSKA_DATE | GRB_MATROSKA_DURATION)

void MatroskaHandler::parseMKV(const std::shared_ptr<CdsItem>& item, std::unique_ptr<MemIOHandler>* pIoHandler)
{
    auto ebmlFile = FileIOCallback(item->getLocation().c_str());
    auto ebmlStream = EbmlStream(ebmlFile);
    activeFlag = GRB_MATROSKA_INFO | GRB_MATROSKA_ARTWORK;

    int iUpperLevel = 0;
    for (auto elL0 = ebmlStream.FindNextID(LIBMATROSKA_NAMESPACE::KaxSegment::ClassInfos, ~0);
         elL0;
         elL0 = ebmlStream.FindNextElement(LIBMATROSKA_NAMESPACE::KaxSegment_Context, iUpperLevel, ~0, true)) {
        iUpperLevel = 0;
        EbmlElement* elL1;
        while ((elL1 = ebmlStream.FindNextElement(elL0->Generic().Context, iUpperLevel, ~0, true))) {
            parseLevel1Element(item, ebmlFile, ebmlStream, elL1, pIoHandler);

            elL1->SkipData(ebmlStream, elL1->Generic().Context);
            delete elL1;
            if (activeFlag == 0) // terminate search
                break;
        } // while elementLevel1

        elL0->SkipData(ebmlStream, LIBMATROSKA_NAMESPACE::KaxSegment_Context);
        delete elL0;
        if (activeFlag == 0) // terminate search
            break;
    } // while elementLevel0

    ebmlFile.close();
}

void MatroskaHandler::parseLevel1Element(const std::shared_ptr<CdsItem>& item, IOCallback& ebmlFile, LIBEBML_NAMESPACE::EbmlStream& ebmlStream, LIBEBML_NAMESPACE::EbmlElement* elL1, std::unique_ptr<MemIOHandler>* pIoHandler)
{
    // Looking at just at EbmlId is not reliable since it can be a dummy element.
    if (!elL1->IsMaster())
        return;
    auto master = dynamic_cast<EbmlMaster*>(elL1);
    if (!master) {
        log_debug("dynamic_cast unexpectedly returned nullptr, seems to be broken");
        return;
    }
    log_debug("MatroskaHandler found {}", EbmlId(*master).GetValue());
    log_debug("MatroskaHandler KaxSeekHead {}", LIBMATROSKA_NAMESPACE::KaxSeekHead::ClassInfos.GlobalId.GetValue());
    log_debug("MatroskaHandler KaxInfo {}", LIBMATROSKA_NAMESPACE::KaxInfo::ClassInfos.GlobalId.GetValue());
    log_debug("MatroskaHandler KaxAttachments {}", LIBMATROSKA_NAMESPACE::KaxAttachments::ClassInfos.GlobalId.GetValue());
    if (EbmlId(*master) == LIBMATROSKA_NAMESPACE::KaxSeekHead::ClassInfos.GlobalId) {
        parseHead(item, ebmlFile, ebmlStream, master, pIoHandler);
        activeFlag = 0; // If there is a head we assume that info and attachments are handled
    } else if (EbmlId(*master) == LIBMATROSKA_NAMESPACE::KaxInfo::ClassInfos.GlobalId) {
        parseInfo(item, ebmlStream, master);
    } else if (EbmlId(*master) == LIBMATROSKA_NAMESPACE::KaxAttachments::ClassInfos.GlobalId) {
        parseAttachments(item, ebmlStream, master, pIoHandler);
    }
}

// This code is inspired by https://github.com/TypesettingTools/DirectASS/blob/master/DSlibass/MatroskaParser.cpp#L58+
void MatroskaHandler::parseHead(const std::shared_ptr<CdsItem>& item, IOCallback& ebmlFile, LIBEBML_NAMESPACE::EbmlStream& ebmlStream, LIBEBML_NAMESPACE::EbmlMaster* info, std::unique_ptr<MemIOHandler>* pIoHandler)
{
    ebmlFile.setFilePointer(0);
    auto estream = LIBEBML_NAMESPACE::EbmlStream(ebmlFile);
    LIBEBML_NAMESPACE::EbmlElement* ebmlHead = estream.FindNextID(EBML_INFO(LIBEBML_NAMESPACE::EbmlHead), ~0);
    ebmlHead->SkipData(estream, EBML_CONTEXT(ebmlHead));
    delete ebmlHead;
    ebmlHead = estream.FindNextID(EBML_INFO(LIBMATROSKA_NAMESPACE::KaxSegment), ~0);

    EbmlElement* dummyEl;
    int iUpperLevel = 0;
    auto metaSeek = static_cast<LIBMATROSKA_NAMESPACE::KaxSeekHead*>(info);

    // master elements
    info->Read(ebmlStream, EBML_CONTEXT(metaSeek), iUpperLevel, dummyEl, true);
    if (!metaSeek->CheckMandatory()) {
        log_debug("parseHead: Some mandatory elements ar missing !!!");
    }
    for (auto&& seekEl : *metaSeek) {
        if (EbmlId(*seekEl) == LIBMATROSKA_NAMESPACE::KaxSeek::ClassInfos.GlobalId) {
            auto seekPoint = static_cast<LIBMATROSKA_NAMESPACE::KaxSeek*>(seekEl);
            LIBMATROSKA_NAMESPACE::KaxSeekID* seekId = nullptr;
            LIBMATROSKA_NAMESPACE::KaxSeekPosition* seekPos = nullptr;
            for (auto&& seekPtEl : *seekPoint) {
                if (EbmlId(*seekPtEl) == LIBMATROSKA_NAMESPACE::KaxSeekID::ClassInfos.GlobalId) {
                    seekId = static_cast<LIBMATROSKA_NAMESPACE::KaxSeekID*>(seekPtEl);
                } else if (EbmlId(*seekPtEl) == LIBMATROSKA_NAMESPACE::KaxSeekPosition::ClassInfos.GlobalId) {
                    seekPos = static_cast<LIBMATROSKA_NAMESPACE::KaxSeekPosition*>(seekPtEl);
                }
            }

            if (!seekId || !seekPos)
                continue;
            auto seekIdValue = EbmlId(seekId->GetBuffer(), seekId->GetSize());
            auto segmentPosition = ebmlHead->GetElementPosition() + ebmlHead->HeadSize() + uint16(*seekPos);
            log_debug("parseHead: Seek ID {} at {}", seekIdValue.GetValue(), segmentPosition);
            if (seekIdValue == LIBMATROSKA_NAMESPACE::KaxAttachments::ClassInfos.GlobalId || seekIdValue == LIBMATROSKA_NAMESPACE::KaxInfo::ClassInfos.GlobalId) {
                ebmlFile.setFilePointer(segmentPosition);
                auto attStream = EbmlStream(ebmlFile);
                int upperLvlElement;
                auto level1 = attStream.FindNextElement(EBML_CLASS_CONTEXT(LIBMATROSKA_NAMESPACE::KaxSegment), upperLvlElement, ~0, true, 1);

                if (EbmlId(*level1) == seekIdValue) {
                    parseLevel1Element(item, ebmlFile, attStream, level1, pIoHandler);
                }
                delete level1;
            }
            if (activeFlag == 0) // terminate search
                break;
        }
    }

    delete ebmlHead;
}

void MatroskaHandler::parseInfo(const std::shared_ptr<CdsItem>& item, LIBEBML_NAMESPACE::EbmlStream& ebmlStream, LIBEBML_NAMESPACE::EbmlMaster* info)
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
            auto title = std::string(UTFstring(*titleEl).GetUTF8());
            log_debug("KaxTitle = {}", title);
            item->addMetaData(M_TITLE, sc->convert(title));
            activeFlag &= ~GRB_MATROSKA_TITLE;
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
                item->addMetaData(M_CREATION_DATE, sc->convert(fDate));
                activeFlag &= ~GRB_MATROSKA_DATE;
            }
        } else if (EbmlId(*el) == LIBMATROSKA_NAMESPACE::KaxDuration::ClassInfos.GlobalId) {
            auto durationEl = dynamic_cast<LIBMATROSKA_NAMESPACE::KaxDuration*>(el);
            if (!durationEl) {
                log_error("Malformed MKV file; KaxDuration cast failed!");
                continue;
            }
            auto fDuration = millisecondsToHMSF(double(*static_cast<EbmlFloat*>(durationEl)));
            log_debug("KaxDuration = {}", fDuration);
            if (item->getResourceCount() > 0) {
                item->getResource(0)->addAttribute(R_DURATION, sc->convert(fDuration));
            }
            activeFlag &= ~GRB_MATROSKA_DURATION;
        }
        if ((activeFlag & GRB_MATROSKA_INFO) == 0) // terminate search
            return;
    }
}

void MatroskaHandler::parseAttachments(const std::shared_ptr<CdsItem>& item, LIBEBML_NAMESPACE::EbmlStream& ebmlStream, LIBEBML_NAMESPACE::EbmlMaster* attachments, std::unique_ptr<MemIOHandler>* pIoHandler)
{
    EbmlElement* dummyEl;
    int iUpperLevel = 0;

    attachments->Read(ebmlStream, EBML_CONTEXT(attachments), iUpperLevel, dummyEl, true);

    for (auto attachedFile = FindChild<LIBMATROSKA_NAMESPACE::KaxAttached>(*attachments);
         attachedFile && (attachedFile->GetSize() > 0);
         attachedFile = &GetNextChild<LIBMATROSKA_NAMESPACE::KaxAttached>(*attachments, *attachedFile)) {
        auto fileName = std::string(UTFstring(GetChild<LIBMATROSKA_NAMESPACE::KaxFileName>(*attachedFile)).GetUTF8());
        log_debug("KaxFileName = {}", fileName);

        if (startswith(fileName, "cover")) {
            const auto& fileData = GetChild<LIBMATROSKA_NAMESPACE::KaxFileData>(*attachedFile);
            log_debug("KaxFileData (size={})", fileData.GetSize());

            if (pIoHandler) {
                // serveContent
                *pIoHandler = std::make_unique<MemIOHandler>(fileData.GetBuffer(), fileData.GetSize());
                activeFlag &= ~GRB_MATROSKA_ARTWORK;
            } else {
                // fillMetadata
                std::string artMimetype = getContentTypeFromByteVector(fileData);
                if (!artMimetype.empty()) {
                    addArtworkResource(item, artMimetype);
                    activeFlag &= ~GRB_MATROSKA_ARTWORK;
                }
            }
        }
        if ((activeFlag & GRB_MATROSKA_ARTWORK) == 0) // terminate search
            return;
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
