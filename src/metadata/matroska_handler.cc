/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    matroska_handler.cc - this file is part of MediaTomb.
    
    Copyright (C) 2019 Patrick Ammann <pammann@gmx.net>
    
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

/// \file matroska_handler.cc
/// \brief Implementeation of the MatroskaHandler class.

#ifdef HAVE_MATROSKA

#include <vector>
#include <iostream>

#include <ebml/IOCallback.h>
#include <ebml/EbmlHead.h>
#include <ebml/EbmlSubHead.h>
#include <ebml/EbmlStream.h>

#include <matroska/KaxSegment.h>
#include <matroska/KaxSeekHead.h>
#include <matroska/KaxCluster.h>
#include <matroska/KaxContexts.h>
#include <matroska/KaxAttached.h>
#include <matroska/KaxAttachments.h>

#include "config/config_manager.h"
#include "iohandler/mem_io_handler.h"
#include "util/string_converter.h"
#include "matroska_handler.h"

#include "util/tools.h"

using namespace zmm;
using namespace LIBEBML_NAMESPACE;
using namespace LIBMATROSKA_NAMESPACE;


// file managment
class file_io_callback: public IOCallback
{
private:
    FILE* file;

public:
    file_io_callback(const char* path)
    {
        file = fopen(path, "rb");
        if (file == nullptr) {
            throw _Exception(std::string("Could not fopen ") + path);
        }
    }

    virtual ~file_io_callback()
    {
        close();
    }

    virtual uint32 read(void * buffer, size_t size)
    {
        assert(file != nullptr);
        if (size <= 0)
            return 0;
        return fread(buffer, 1, size, file);
    }

    virtual void setFilePointer(int64_t offset, seek_mode mode = seek_beginning)
    {
        assert(file != nullptr);
        assert(mode == SEEK_CUR || mode == SEEK_END || mode == SEEK_SET);
        if (fseeko(file, offset, mode) != 0) {
            throw _Exception("fseek failed");
        }
    }

    virtual size_t write(const void *p_buffer, size_t i_size)
    {
        // not needed
        return 0;
    }

    virtual uint64 getFilePointer(void)
    {
        assert(file != nullptr);
        return ftello(file);
    }

    virtual void close(void)
    {
        if (file == nullptr)
            return;
        if (fclose(file) !=0) {
            throw _Exception("fclose failed");
        }
        file = nullptr;
    }
};


MatroskaHandler::MatroskaHandler(std::shared_ptr<ConfigManager> config)
    : MetadataHandler(config)
{
}

void MatroskaHandler::fillMetadata(Ref<CdsItem> item)
{
    parseMKV(item, NULL);
}

std::unique_ptr<IOHandler> MatroskaHandler::serveContent(Ref<CdsItem> item, int resNum)
{
    MemIOHandler* io_handler;
    parseMKV(item, &io_handler);

    std::unique_ptr<IOHandler> h;
    h.reset(io_handler);
    return h;
}

void MatroskaHandler::parseMKV(Ref<CdsItem> item, MemIOHandler** p_io_handler)
{
    file_io_callback ebml_file(item->getLocation().c_str());
    EbmlStream ebml_stream(ebml_file);

    EbmlElement * el_l0 = ebml_stream.FindNextID(KaxSegment::ClassInfos, ~0);
    while (el_l0 != NULL) {
        int i_upper_level = 0;
        EbmlElement * el_l1 = ebml_stream.FindNextElement(el_l0->Generic().Context, i_upper_level, ~0, true);
        while (el_l1 != NULL) {
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

void MatroskaHandler::parseLevel1Element(zmm::Ref<CdsItem> item, EbmlStream & ebml_stream, EbmlElement * el_l1, MemIOHandler** p_io_handler)
{
    if (EbmlId(*el_l1) == KaxInfo::ClassInfos.GlobalId) {
        parseInfo(item, ebml_stream, static_cast<KaxInfo *>(el_l1));
    } else if (EbmlId(*el_l1) == KaxAttachments::ClassInfos.GlobalId) {
        parseAttachments(item, ebml_stream, static_cast<KaxAttachments *>(el_l1), p_io_handler);
    }
}

void MatroskaHandler::parseInfo(Ref<CdsItem> item, EbmlStream & ebml_stream, KaxInfo *info)
{
    EbmlElement* dummy_el;
    int i_upper_level = 0;
    EbmlMaster  *m;

    // master elements
    m = static_cast<EbmlMaster *>(info);
    m->Read(ebml_stream, EBML_CONTEXT(info), i_upper_level, dummy_el, true);

    Ref<StringConverter> sc = StringConverter::i2i(config); // sure is sure

    for( size_t i = 0; i < m->ListSize(); i++) {
        EbmlElement* el = (*m)[i];

        if (EbmlId(*el) == KaxTitle::ClassInfos.GlobalId) {
            std::string title(UTFstring(*static_cast<KaxTitle *>(el)).GetUTF8());
            // printf("KaxTitle = %s\n", title.c_str());
            item->setMetadata(MT_KEYS[M_TITLE].upnp, sc->convert(title));
        } else if (EbmlId(*el) == KaxDateUTC::ClassInfos.GlobalId) {
            KaxDateUTC &date = *static_cast<KaxDateUTC *>(el);
            time_t i_date;
            struct tm tmres;
            char   buffer[25];
            i_date = date.GetEpochDate();
            if (gmtime_r(&i_date, &tmres) &&
                strftime(buffer, sizeof(buffer), "%Y-%m-%d", &tmres))
            {
                // printf("KaxDateUTC = %s\n", buffer);
                item->setMetadata(MT_KEYS[M_DATE].upnp, sc->convert(buffer));
            }
        }
    }
}

void MatroskaHandler::parseAttachments(Ref<CdsItem> item, EbmlStream & ebml_stream, KaxAttachments *attachments, MemIOHandler** p_io_handler)
{
    EbmlElement* dummy_el;
    int i_upper_level = 0;

    attachments->Read(ebml_stream, EBML_CONTEXT(attachments), i_upper_level, dummy_el, true);

    KaxAttached* attachedFile = FindChild<KaxAttached>(*attachments);
    while (attachedFile && (attachedFile->GetSize() > 0)) {
        std::string fileName(UTFstring(GetChild<KaxFileName>(*attachedFile)).GetUTF8());
        // printf("KaxFileName = %s\n", fileName.c_str());

        bool isCoverArt = false;
        if (startswith(fileName, "cover")) {
            isCoverArt = true;
        }

        if (isCoverArt) {
            KaxFileData& fileData = GetChild<KaxFileData>(*attachedFile);
            // printf("KaxFileData (size=%ld)\n", fileData.GetSize());

            if (p_io_handler != NULL) {
                // serveContent
                *p_io_handler = new MemIOHandler(fileData.GetBuffer(), fileData.GetSize());
            } else {
                // fillMetadata
                std::string art_mimetype = getContentTypeFromByteVector(&fileData);
                if (string_ok(art_mimetype))
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
    art_mimetype = getMIMETypeFromBuffer(data->GetBuffer(), data->GetSize());
    if (!string_ok(art_mimetype)) {
        return MIMETYPE_DEFAULT;
    }
#endif
    return art_mimetype;
}

void MatroskaHandler::addArtworkResource(Ref<CdsItem> item, std::string art_mimetype)
{
    // if we could not determine the mimetype, then there is no
    // point to add the resource - it's probably garbage
    log_debug("Found artwork of type %s in file %s\n", art_mimetype.c_str(), item->getLocation().c_str());

    if (art_mimetype != MIMETYPE_DEFAULT) {
        Ref<CdsResource> resource(new CdsResource(CH_MATROSKA));
        resource->addAttribute(
            MetadataHandler::getResAttrName(R_PROTOCOLINFO),
            renderProtocolInfo(art_mimetype));
        resource->addParameter(
            RESOURCE_CONTENT_TYPE,
            ID3_ALBUM_ART);
        item->addResource(resource);
    }
}

#endif // HAVE_MATROSKA
