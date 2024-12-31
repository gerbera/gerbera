/*GRB*

    Gerbera - https://gerbera.io/

    matroska_handler.cc - this file is part of Gerbera.

    Copyright (C) 2019-2025 Gerbera Contributors

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
#define GRB_LOG_FAC GrbLogFacility::matroska
#include "matroska_handler.h" // API

#include "cds/cds_item.h"
#include "config/config_val.h"
#include "exceptions.h"
#include "iohandler/mem_io_handler.h"
#include "util/grb_time.h"
#include "util/mime.h"
#include "util/string_converter.h"
#include "util/tools.h"

#include <fmt/chrono.h>
#include <vector>

#include <ebml/EbmlHead.h>
#include <ebml/EbmlStream.h>
#include <ebml/IOCallback.h>

#if LIBMATROSKA_VERSION < 0x020000
#include <matroska/KaxAttachments.h>
#endif
#include <matroska/KaxContexts.h>
#include <matroska/KaxSeekHead.h>
#include <matroska/KaxSegment.h>

using namespace libebml;
using namespace libmatroska;

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

#if LIBMATROSKA_VERSION < 0x020000
    uint32
#else
    std::size_t
#endif
    read(void* buffer, std::size_t size) override
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

    std::uint64_t getFilePointer() override
    {
        assert(mediaFile);
        return ftello(mediaFile);
    }

    void close() override
    {
        mediaFile = nullptr;
    }
};

MatroskaHandler::MatroskaHandler(const std::shared_ptr<Context>& context)
    : MediaMetadataHandler(context, ConfigVal::IMPORT_LIBOPTS_MKV_ENABLED)
{
}

void MatroskaHandler::fillMetadata(const std::shared_ptr<CdsObject>& obj)
{
    auto item = std::dynamic_pointer_cast<CdsItem>(obj);
    if (!item || !isEnabled)
        return;

    parseMKV(item, nullptr);
}

std::unique_ptr<IOHandler> MatroskaHandler::serveContent(const std::shared_ptr<CdsObject>& obj, const std::shared_ptr<CdsResource>& resource)
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
    for (auto elL0 = ebmlStream.FindNextID(EBML_INFO(KaxSegment), ~0);
         elL0;
         elL0 = ebmlStream.FindNextElement(EBML_CLASS_CONTEXT(KaxSegment), iUpperLevel, ~0, true)) {
        iUpperLevel = 0;
        EbmlElement* elL1;
        while ((elL1 = ebmlStream.FindNextElement(EBML_CONTEXT(elL0), iUpperLevel, ~0, true))) {
            parseLevel1Element(item, ebmlFile, ebmlStream, elL1, pIoHandler);

            delete (elL1->SkipData(ebmlStream, EBML_CONTEXT(elL1)));
            delete elL1;
            if (activeFlag == 0) // terminate search
                break;
        } // while elementLevel1

        delete (elL0->SkipData(ebmlStream, EBML_CLASS_CONTEXT(KaxSegment)));
        delete elL0;
        if (activeFlag == 0) // terminate search
            break;
    } // while elementLevel0

    ebmlFile.close();
}

void MatroskaHandler::parseLevel1Element(const std::shared_ptr<CdsItem>& item, IOCallback& ebmlFile, EbmlStream& ebmlStream, EbmlElement* elL1, std::unique_ptr<MemIOHandler>* pIoHandler)
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
    log_debug("MatroskaHandler KaxSeekHead {}", EBML_ID(KaxSeekHead).GetValue());
    log_debug("MatroskaHandler KaxInfo {}", EBML_ID(KaxInfo).GetValue());
    log_debug("MatroskaHandler KaxAttachments {}", EBML_ID(KaxAttachments).GetValue());
    if (EbmlId(*master) == EBML_ID(KaxSeekHead)) {
        parseHead(item, ebmlFile, ebmlStream, master, pIoHandler);
        activeFlag = 0; // If there is a head we assume that info and attachments are handled
    } else if (EbmlId(*master) == EBML_ID(KaxInfo)) {
        parseInfo(item, ebmlStream, master);
    } else if (EbmlId(*master) == EBML_ID(KaxAttachments)) {
        parseAttachments(item, ebmlStream, master, pIoHandler);
    }
}

// This code is inspired by https://github.com/TypesettingTools/DirectASS/blob/master/DSlibass/MatroskaParser.cpp#L58+
void MatroskaHandler::parseHead(const std::shared_ptr<CdsItem>& item, IOCallback& ebmlFile, EbmlStream& ebmlStream, EbmlMaster* info, std::unique_ptr<MemIOHandler>* pIoHandler)
{
    auto metaSeek = dynamic_cast<KaxSeekHead*>(info);
    if (!metaSeek)
        return;

    ebmlFile.setFilePointer(0);
    auto estream = EbmlStream(ebmlFile);
    EbmlElement* ebmlHead = estream.FindNextID(EBML_INFO(EbmlHead), ~0);
    delete (ebmlHead->SkipData(estream, EBML_CONTEXT(ebmlHead)));
    delete ebmlHead;

    EbmlElement* dummyEl;
    int iUpperLevel = 0;

    ebmlHead = estream.FindNextID(EBML_INFO(KaxSegment), ~0);
    // master elements
    info->Read(ebmlStream, EBML_CONTEXT(metaSeek), iUpperLevel, dummyEl, true);
    if (!metaSeek->CheckMandatory()) {
        log_debug("parseHead: Some mandatory elements ar missing !!!");
    }
    for (auto&& seekEl : *metaSeek) {
        if (EbmlId(*seekEl) == EBML_ID(KaxSeek)) {
            auto seekPoint = dynamic_cast<KaxSeek*>(seekEl);
            if (!seekPoint)
                break;

            KaxSeekID* seekId = nullptr;
            KaxSeekPosition* seekPos = nullptr;
            for (auto&& seekPtEl : *seekPoint) {
                if (EbmlId(*seekPtEl) == EBML_ID(KaxSeekID)) {
                    seekId = dynamic_cast<KaxSeekID*>(seekPtEl);
                } else if (EbmlId(*seekPtEl) == EBML_ID(KaxSeekPosition)) {
                    seekPos = dynamic_cast<KaxSeekPosition*>(seekPtEl);
                }
            }

            if (!seekId || !seekPos)
                continue;
#if LIBMATROSKA_VERSION < 0x020000
            auto seekIdValue = EbmlId(seekId->GetBuffer(), seekId->GetSize());
#else
            auto seekIdValue = EbmlId::FromBuffer(seekId->GetBuffer(), seekId->GetSize());
#endif
            auto segmentPosition = static_cast<EbmlMaster*>(ebmlHead)->GetDataStart() + static_cast<std::uint16_t>(*seekPos);
            log_debug("parseHead: Seek ID {} at {}", seekIdValue.GetValue(), segmentPosition);
            if (seekPoint->IsEbmlId(EBML_ID(KaxAttachments)) || seekPoint->IsEbmlId(EBML_ID(KaxInfo))) {
                ebmlFile.setFilePointer(segmentPosition);
                auto attStream = EbmlStream(ebmlFile);
                int upperLvlElement = 0;
                auto level1 = attStream.FindNextElement(EBML_CLASS_CONTEXT(KaxSegment), upperLvlElement, ~0, true, 1);

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

void MatroskaHandler::parseInfo(const std::shared_ptr<CdsItem>& item, EbmlStream& ebmlStream, EbmlMaster* info)
{
    EbmlElement* dummyEl;
    int iUpperLevel = 0;

    // master elements
    info->Read(ebmlStream, EBML_CONTEXT(info), iUpperLevel, dummyEl, true);

    auto sc = converterManager->m2i(ConfigVal::IMPORT_LIBOPTS_MKV_CHARSET, item->getLocation());
    for (auto&& el : *info) {
        if (EbmlId(*el) == EBML_ID(KaxTitle)) {
            auto titleEl = dynamic_cast<KaxTitle*>(el);
            if (!titleEl) {
                log_error("Malformed MKV file; KaxTitle cast failed!");
                continue;
            }
            auto title = std::string(UTFstring(*titleEl).GetUTF8());
            log_debug("KaxTitle = {}", title);
            item->addMetaData(MetadataFields::M_TITLE, sc->convert(title));
            activeFlag &= ~GRB_MATROSKA_TITLE;
        } else if (EbmlId(*el) == EBML_ID(KaxDateUTC)) {
            auto dateEl = dynamic_cast<KaxDateUTC*>(el);
            if (!dateEl) {
                log_error("Malformed MKV file; KaxDateUTC cast failed!");
                continue;
            }
            auto fDate = fmt::format("{:%FT%T%z}", fmt::gmtime(dateEl->GetEpochDate()));
            if (!fDate.empty()) {
                log_debug("KaxDateUTC = {}", fDate);
                item->addMetaData(MetadataFields::M_DATE, sc->convert(fDate));
                item->addMetaData(MetadataFields::M_CREATION_DATE, sc->convert(fDate));
                activeFlag &= ~GRB_MATROSKA_DATE;
            }
        } else if (EbmlId(*el) == EBML_ID(KaxDuration)) {
            auto durationEl = dynamic_cast<KaxDuration*>(el);
            if (!durationEl) {
                log_error("Malformed MKV file; KaxDuration cast failed!");
                continue;
            }
            auto fDuration = millisecondsToHMSF(static_cast<double>(*static_cast<EbmlFloat*>(durationEl)));
            log_debug("KaxDuration = {}", fDuration);
            if (item->getResourceCount() > 0) {
                item->getResource(ContentHandler::DEFAULT)->addAttribute(ResourceAttribute::DURATION, sc->convert(fDuration));
            }
            activeFlag &= ~GRB_MATROSKA_DURATION;
        }
        if ((activeFlag & GRB_MATROSKA_INFO) == 0) // terminate search
            return;
    }
}

void MatroskaHandler::parseAttachments(const std::shared_ptr<CdsItem>& item, EbmlStream& ebmlStream, EbmlMaster* attachments, std::unique_ptr<MemIOHandler>* pIoHandler)
{
    EbmlElement* dummyEl;
    int iUpperLevel = 0;

    attachments->Read(ebmlStream, EBML_CONTEXT(attachments), iUpperLevel, dummyEl, true);

    for (auto attachedFile = FindChild<KaxAttached>(*attachments);
         attachedFile && (attachedFile->GetSize() > 0);
         attachedFile = &GetNextChild<KaxAttached>(*attachments, *attachedFile)) {
        auto fileName = std::string(UTFstring(GetChild<KaxFileName>(*attachedFile)).GetUTF8());
        log_debug("KaxFileName = {}", fileName);

        if (startswith(fileName, "cover")) {
            const auto& fileData = GetChild<KaxFileData>(*attachedFile);
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

std::string MatroskaHandler::getContentTypeFromByteVector(const KaxFileData& data) const
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
        auto resource = std::make_shared<CdsResource>(ContentHandler::MATROSKA, ResourcePurpose::Thumbnail);
        resource->addAttribute(ResourceAttribute::PROTOCOLINFO, renderProtocolInfo(artMimetype));
        item->addResource(resource);
    }
}

#endif // HAVE_MATROSKA
