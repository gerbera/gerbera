/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    cds_resource_manager.cc - this file is part of MediaTomb.
    
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

/// \file cds_resource_manager.cc

#include "cds_resource_manager.h"
#include "dictionary.h"
#include "object_dictionary.h"
#include "server.h"
#include "common.h"
#include "tools.h"
#include "metadata_handler.h"

using namespace zmm;
using namespace mxml;

CdsResourceManager::CdsResourceManager() : Object()
{
}

String CdsResourceManager::renderExtension(String contentType, String location)
{
    String ext = _(_URL_PARAM_SEPARATOR) + URL_FILE_EXTENSION + 
                _URL_PARAM_SEPARATOR + "file";

    if (string_ok(contentType) && (contentType != CONTENT_TYPE_PLAYLIST))
    {
        ext = ext + _(".") + contentType;
        return ext;
    }

    if (string_ok(location))
    {
        int dot = location.rindex('.');
        if (dot > -1)
        {
            String extension = location.substring(dot);
            // make sure that the extension does not contain the separator character
            if (string_ok(extension) && 
               (extension.index(URL_PARAM_SEPARATOR) == -1) &&
               (extension.index(URL_PARAM_SEPARATOR) == -1))
            {
                ext = ext + extension;
                return ext;
            }
        }
    }
    return nil;
}

void CdsResourceManager::addResources(Ref<CdsItem> item, Ref<Element> element)
{
    Ref<UrlBase> urlBase = addResources_getUrlBase(item);
    Ref<ConfigManager> config = ConfigManager::getInstance();
    bool skipURL = ((IS_CDS_ITEM_INTERNAL_URL(item->getObjectType()) || 
                    IS_CDS_ITEM_EXTERNAL_URL(item->getObjectType())) &&
                    (!item->getFlag(OBJECT_FLAG_PROXY_URL)));

    bool isExtThumbnail = false; // this sucks
    Ref<Dictionary> mappings = config->getDictionaryOption(
                        CFG_IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST);

#if defined(HAVE_FFMPEG) && defined(HAVE_FFMPEGTHUMBNAILER)
    if (config->getBoolOption(CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_ENABLED) && 
       (item->getMimeType().startsWith(_("video")) || 
        item->getFlag(OBJECT_FLAG_OGG_THEORA)))
    {
        String videoresolution = item->getResource(0)->getAttribute(MetadataHandler::getResAttrName(R_RESOLUTION));
        int x;
        int y;

        if (string_ok(videoresolution) && 
            check_resolution(videoresolution, &x, &y))
        {
            String thumb_mimetype = mappings->get(_(CONTENT_TYPE_JPG));
            if (!string_ok(thumb_mimetype))
                thumb_mimetype = _("image/jpeg");

            Ref<CdsResource> ffres(new CdsResource(CH_FFTH));
            ffres->addParameter(_(RESOURCE_HANDLER), String::from(CH_FFTH));
            ffres->addAttribute(MetadataHandler::getResAttrName(R_PROTOCOLINFO),
                    renderProtocolInfo(thumb_mimetype));
            ffres->addOption(_(RESOURCE_CONTENT_TYPE), _(THUMBNAIL));

            y = config->getIntOption(CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_THUMBSIZE) * y / x;
            x = config->getIntOption(CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_THUMBSIZE);
            String resolution = String::from(x) + "x" + String::from(y);
            ffres->addAttribute(MetadataHandler::getResAttrName(R_RESOLUTION),
                    resolution);
            item->addResource(ffres);
            log_debug("Adding resource for video thumbnail\n");
        }
    }
#endif // FFMPEGTHUMBNAILER

#ifdef EXTERNAL_TRANSCODING
    // this will be used to count only the "real" resources, omitting the
    // transcoded ones
    int realCount = 0;
    bool hide_original_resource = false;
    int original_resource = 0;
    
    Ref<UrlBase> urlBase_tr;

    // once proxying is a feature that can be turned off or on in
    // config manager we should use that setting 
    //
    // TODO: allow transcoding for URLs
        
    // now get the profile
    Ref<TranscodingProfileList> tlist = config->getTranscodingProfileListOption(
            CFG_TRANSCODING_PROFILE_LIST);
    Ref<ObjectDictionary<TranscodingProfile> > tp_mt = tlist->get(item->getMimeType());
    if (tp_mt != nil)
    {
        Ref<Array<ObjectDictionaryElement<TranscodingProfile> > > profiles = tp_mt->getElements();
        for (int p = 0; p < profiles->size(); p++)
        {
            Ref<TranscodingProfile> tp = profiles->get(p)->getValue();

            if (tp == nil)
                throw _Exception(_("Invalid profile encountered!"));

            String ct = mappings->get(item->getMimeType());
            if (ct == CONTENT_TYPE_OGG) 
            {
                if (((item->getFlag(OBJECT_FLAG_OGG_THEORA)) && 
                     (!tp->isTheora())) ||
                    (!item->getFlag(OBJECT_FLAG_OGG_THEORA) && 
                    (tp->isTheora())))
                     {
                         continue;
                     }
            }
            // check user fourcc settings
            else if (ct == CONTENT_TYPE_AVI)
            {
                avi_fourcc_listmode_t fcc_mode = tp->getAVIFourCCListMode();

                Ref<Array<StringBase> > fcc_list = tp->getAVIFourCCList();
                // mode is either process or ignore, so we will have to take a
                // look at the settings
                if (fcc_mode != FCC_None)
                {
                    String current_fcc = item->getResource(0)->getOption(_(RESOURCE_OPTION_FOURCC));
                    // we can not do much if the item has no fourcc info,
                    // so we will transcode it anyway
                    if (!string_ok(current_fcc))
                    {
                        // the process mode specifies that we will transcode
                        // ONLY if the fourcc matches the list; since an invalid
                        // fourcc can not match anything we will skip the item
                        if (fcc_mode == FCC_Process)
                            continue;
                    }
                    // we have the current and hopefully valid fcc string
                    // let's have a look if it matches the list
                    else
                    {
                        bool fcc_match = false;
                        for (int f = 0; f < fcc_list->size(); f++)
                        {
                            if (current_fcc == fcc_list->get(f)->data)
                                fcc_match = true;
                        }
                       
                        if (!fcc_match && (fcc_mode == FCC_Process))
                            continue;

                        if (fcc_match && (fcc_mode == FCC_Ignore))
                            continue;
                    }
                }
            }

            if (!item->getFlag(OBJECT_FLAG_DVD_IMAGE) && (tp->onlyDVD()))
                continue;

            Ref<CdsResource> t_res(new CdsResource(CH_TRANSCODE));
            t_res->addParameter(_(URL_PARAM_TRANSCODE_PROFILE_NAME), tp->getName());
            // after transcoding resource was added we can not rely on
            // index 0, so we will make sure the ogg option is there
            t_res->addOption(_(CONTENT_TYPE_OGG), 
                         item->getResource(0)->getOption(_(CONTENT_TYPE_OGG)));
            t_res->addParameter(_(URL_PARAM_TRANSCODE), _(URL_VALUE_TRANSCODE));

            String targetMimeType = tp->getTargetMimeType();

            if (!tp->isThumbnail())
            {
                // duration should be the same for transcoded media, so we can 
                // take the value from the original resource
                String duration = item->getResource(0)->getAttribute(MetadataHandler::getResAttrName(R_DURATION));
                if (string_ok(duration))
                    t_res->addAttribute(MetadataHandler::getResAttrName(R_DURATION),
                            duration);

                int freq = tp->getSampleFreq();
                if (freq == SOURCE)
                {
                    String frequency = item->getResource(0)->getAttribute(MetadataHandler::getResAttrName(R_SAMPLEFREQUENCY));
                    if (string_ok(frequency))
                    {
                        t_res->addAttribute(MetadataHandler::getResAttrName(R_SAMPLEFREQUENCY), frequency);
                        targetMimeType = targetMimeType + _(";rate=") + 
                                         frequency;
                    }
                }
                else if (freq != OFF)
                {
                    t_res->addAttribute(MetadataHandler::getResAttrName(R_SAMPLEFREQUENCY), String::from(freq));
                    targetMimeType = targetMimeType + _(";rate=") + 
                                     String::from(freq);
                }

                int chan = tp->getNumChannels();
                if (chan == SOURCE)
                {
                    String nchannels = item->getResource(0)->getAttribute(MetadataHandler::getResAttrName(R_NRAUDIOCHANNELS));
                    if (string_ok(nchannels))
                    {
                        t_res->addAttribute(MetadataHandler::getResAttrName(R_NRAUDIOCHANNELS), nchannels);
                        targetMimeType = targetMimeType + _(";channels=") + 
                                         nchannels;
                    }
                }
                else if (chan != OFF)
                {
                    t_res->addAttribute(MetadataHandler::getResAttrName(R_NRAUDIOCHANNELS), String::from(chan));
                    targetMimeType = targetMimeType + _(";channels=") + 
                                     String::from(chan);
                }
            }

            t_res->addAttribute(MetadataHandler::getResAttrName(R_PROTOCOLINFO),
                    renderProtocolInfo(targetMimeType));

            if (tp->isThumbnail())
                t_res->addOption(_(RESOURCE_CONTENT_TYPE), _(EXIF_THUMBNAIL));

            t_res->mergeAttributes(tp->getAttributes());

            if (tp->hideOriginalResource())
                hide_original_resource = true;

            if (tp->firstResource())
            {
                item->insertResource(0, t_res);
                original_resource++;
            }
            else
                item->addResource(t_res);
        }

        if (skipURL)
            urlBase_tr = addResources_getUrlBase(item, true);
    }

#endif // EXTERNAL_TRANSCODING

    int resCount = item->getResourceCount();
    for (int i = 0; i < resCount; i++)
    {
        /// \todo what if the resource has a different mimetype than the item??
        /*        String mimeType = item->getMimeType();
                  if (!string_ok(mimeType)) mimeType = DEFAULT_MIMETYPE; */

        Ref<Dictionary> res_attrs = item->getResource(i)->getAttributes();
        Ref<Dictionary> res_params = item->getResource(i)->getParameters();
        String protocolInfo = res_attrs->get(MetadataHandler::getResAttrName(R_PROTOCOLINFO));
        String mimeType = getMTFromProtocolInfo(protocolInfo);

        int pos = mimeType.find(";");
        if (pos != -1)
        {
            mimeType = mimeType.substring(0, pos);
        }

        assert(string_ok(mimeType));
        String contentType = mappings->get(mimeType);
        String url;

        /// \todo who will sync mimetype that is part of the protocl info and
        /// that is lying in the resources with the information that is in the
        /// resource tags?

        // ok, here is the tricky part:
        // we add transcoded resources dynamically, that means that when 
        // the object is loaded from storage those resources are not there;
        // this again means, that we have to add the res_id parameter
        // accounting for those dynamic resources: i.e. the parameter should
        // still only count the "real" resources, because that's what the
        // file request handler will be getting.
        // for transcoded resources the res_id can be safely ignored,
        // because a transcoded resource is identified by the profile name
#ifdef EXTERNAL_TRANSCODING
        // flag if we are dealing with the transcoded resource
        bool transcoded = (res_params->get(_(URL_PARAM_TRANSCODE)) == URL_VALUE_TRANSCODE);
        if (!transcoded)
        {
            if (urlBase->addResID)
            {
                url = urlBase->urlBase + realCount;
            }
            else
                url = urlBase->urlBase;

            realCount++;
        }
        else
        {
            if (!skipURL)
                url = urlBase->urlBase + _(URL_VALUE_TRANSCODE_NO_RES_ID);
            else
            {
                assert(urlBase_tr != nil);
                url = urlBase_tr->urlBase + _(URL_VALUE_TRANSCODE_NO_RES_ID);
            }
        }
#else
        if (urlBase->addResID)
            url = urlBase->urlBase + i;
        else
            url = urlBase->urlBase;
#endif
        if ((res_params != nil) && (res_params->size() > 0))
        {
            url = url + _(_URL_PARAM_SEPARATOR);
            url = url + res_params->encodeSimple();
        }

        // ok this really sucks, I guess another rewrite of the resource manager
        // is necessary
        if ((i > 0) && (item->getResource(i)->getHandlerType() == CH_EXTURL) &&
           ((item->getResource(i)->getOption(_(RESOURCE_CONTENT_TYPE)) == 
            THUMBNAIL) || 
            (item->getResource(i)->getOption(_(RESOURCE_CONTENT_TYPE)) ==
                            ID3_ALBUM_ART)))
        {
            url = item->getResource(i)->getOption(_(RESOURCE_OPTION_URL));
            if (!string_ok(url))
                throw _Exception(_("missing thumbnail URL!"));

            isExtThumbnail = true;
        }

        /// \todo currently resource is misused for album art
#ifdef HAVE_ID3_ALBUMART
        // only add upnp:AlbumArtURI if we have an AA, skip the resource
        if ((i > 0) && ((item->getResource(i)->getHandlerType() == CH_ID3) ||
                        (item->getResource(i)->getHandlerType() == CH_MP4) ||
                        (item->getResource(i)->getHandlerType() == CH_FLAC) ||
                        (item->getResource(i)->getHandlerType() == CH_FANART) ||
                        (item->getResource(i)->getHandlerType() == CH_EXTURL)))
        {
            String rct;
            if (item->getResource(i)->getHandlerType() == CH_EXTURL)
                rct = item->getResource(i)->getOption(_(RESOURCE_CONTENT_TYPE));
            else    
                rct = item->getResource(i)->getParameter(_(RESOURCE_CONTENT_TYPE));
            if (rct == ID3_ALBUM_ART)
            {
                Ref<Element> aa(new Element(MetadataHandler::getMetaFieldName(M_ALBUMARTURI)));
                aa->setText(url);
#ifdef EXTEND_PROTOCOLINFO
                if (config->getBoolOption(CFG_SERVER_EXTEND_PROTOCOLINFO))
                {
                    /// \todo clean this up, make sure to check the mimetype and
                    /// provide the profile correctly
                    aa->setAttribute(_("xmlns:dlna"), 
                                     _("urn:schemas-dlna-org:metadata-1-0"));
                    aa->setAttribute(_("dlna:profileID"), _("JPEG_TN"));
                }
#endif
                element->appendElementChild(aa);
                continue;
            }
        }
#endif
        if (!isExtThumbnail)
        {
#ifdef EXTERNAL_TRANSCODING
            // when transcoding is enabled the first (zero) resource can be the
            // transcoded stream, that means that we can only go with the
            // content type here and that we will not limit ourselves to the
            // first resource
            if (!skipURL)
            {
                if (transcoded)
                    url = url + renderExtension(contentType, nil);
                else
                    url = url + renderExtension(contentType, item->getLocation()); 
            }
#else
        // for non transcoded item we will only do this for the first
        // resource, that seemed to be sufficient so far (mostly this paramter
        // is implemented for the TG100 renderer that will not play .avi files
        // otherwise
        if ((i == 0) && (!skipURL))
        {
            url = url + renderExtension(contentType, item->getLocation());
        }

#endif
        }
#ifdef EXTEND_PROTOCOLINFO
        if (config->getBoolOption(CFG_SERVER_EXTEND_PROTOCOLINFO))
        {
            String extend;
            if (contentType == CONTENT_TYPE_MP3)
                extend = _(D_PROFILE) + "=" + D_MP3 + ";";
            else if (contentType == CONTENT_TYPE_PCM)
                extend = _(D_PROFILE) + "=" + D_LPCM + ";";
            else if (contentType == CONTENT_TYPE_JPG)
            {
                String resolution = res_attrs->get(MetadataHandler::getResAttrName(R_RESOLUTION));
                int x;
                int y;
                if (string_ok(resolution) && 
                    check_resolution(resolution, &x, &y))
                {

          if ((i > 0) && 
              (((item->getResource(i)->getHandlerType() == CH_LIBEXIF) && 
               (item->getResource(i)->getParameter(_(RESOURCE_CONTENT_TYPE)) 
                     == EXIF_THUMBNAIL)) || 
               (item->getResource(i)->getOption(_(RESOURCE_CONTENT_TYPE)) 
                     == EXIF_THUMBNAIL) || 
               (item->getResource(i)->getOption(_(RESOURCE_CONTENT_TYPE))
                                              == THUMBNAIL)) &&
              (x <= 160) && (y <= 160))
                        extend = _(D_PROFILE) + "=" + D_JPEG_TN+";";
                    else if ((x <= 640) && (y <= 420))
                        extend = _(D_PROFILE) + "=" + D_JPEG_SM+";";
                    else if ((x <= 1024) && (y <=768))
                        extend = _(D_PROFILE) + "=" + D_JPEG_MED+";";
                    else if ((x <= 4096) && (y <=4096))
                        extend = _(D_PROFILE) + "=" + D_JPEG_LRG+";";
                }
            }

#ifdef EXTERNAL_TRANSCODING
        // we do not support seeking at all, so 00
        // and the media is converted, so set CI to 1
        if (!isExtThumbnail && transcoded)
        {
            extend = extend + D_OP + "=10;" +
                     D_CONVERSION_INDICATOR + "=" D_CONVERSION;

            if (mimeType.startsWith(_("audio")) || 
                mimeType.startsWith(_("video")))
                extend = extend + ";" D_FLAGS "=" D_TR_FLAGS_AV;
        }
        else
#endif
        extend = extend + D_OP + "=01;" + 
                 D_CONVERSION_INDICATOR + "=" + D_NO_CONVERSION;

        protocolInfo = protocolInfo.substring(0, protocolInfo.rindex(':')+1) +
                       extend;
        res_attrs->put(MetadataHandler::getResAttrName(R_PROTOCOLINFO),
                       protocolInfo);

        if (config->getBoolOption(CFG_SERVER_EXTEND_PROTOCOLINFO_SM_HACK))
        {
            if (mimeType.startsWith(_("video")))
            {
                element->appendElementChild(UpnpXML_DIDLRenderCaptionInfo(url));
            }
        }

        log_debug("extended protocolInfo: %s\n", protocolInfo.c_str());
        }
#endif
#ifdef EXTERNAL_TRANSCODING
        if (!hide_original_resource || transcoded || 
           (hide_original_resource && (original_resource != i)))
#endif
            element->appendElementChild(UpnpXML_DIDLRenderResource(url, res_attrs));
    }
}

Ref<CdsResourceManager::UrlBase> CdsResourceManager::addResources_getUrlBase(Ref<CdsItem> item, bool forceLocal)
{
    Ref<Element> res;

    Ref<UrlBase> urlBase(new UrlBase);
    /// \todo resource options must be read from configuration files

    Ref<Dictionary> dict(new Dictionary());
    dict->put(_(URL_OBJECT_ID), String::from(item->getID()));

    urlBase->addResID = false;
    /// \todo move this down into the "for" loop and create different urls 
    /// for each resource once the io handlers are ready
    int objectType = item->getObjectType();
    if (IS_CDS_ITEM_INTERNAL_URL(objectType))
    {
        urlBase->urlBase = Server::getInstance()->getVirtualURL() + 
                           _(_URL_PARAM_SEPARATOR) + 
                           CONTENT_SERVE_HANDLER + 
                           _(_URL_PARAM_SEPARATOR) + item->getLocation();
        return urlBase;
    }

    if (IS_CDS_ITEM_EXTERNAL_URL(objectType))
    {
        if (!item->getFlag(OBJECT_FLAG_PROXY_URL) && (!forceLocal))
        {
            urlBase->urlBase = item->getLocation();
            return urlBase;
        }

        if ((item->getFlag(OBJECT_FLAG_ONLINE_SERVICE) && 
                item->getFlag(OBJECT_FLAG_PROXY_URL)) || forceLocal)
        {
            urlBase->urlBase = Server::getInstance()->getVirtualURL() + 
                _(_URL_PARAM_SEPARATOR) +
                CONTENT_ONLINE_HANDLER + _(_URL_PARAM_SEPARATOR) +
                dict->encodeSimple() + _(_URL_PARAM_SEPARATOR) +
                _(URL_RESOURCE_ID) + _(_URL_PARAM_SEPARATOR);
            urlBase->addResID = true;
            return urlBase;
        }
    }

#ifdef HAVE_LIBDVDNAV_DISABLED
    if (IS_CDS_ITEM(objectType) && (item->getFlag(OBJECT_FLAG_DVD_IMAGE)))
    {
        urlBase->urlBase = Server::getInstance()->getVirtualURL() +
                          _(_URL_PARAM_SEPARATOR) +
                           CONTENT_DVD_IMAGE_HANDLER + _(_URL_PARAM_SEPARATOR) +
                           dict->encodeSimple() + _(_URL_PARAM_SEPARATOR) +
                           _(URL_RESOURCE_ID) + _(_URL_PARAM_SEPARATOR);
        urlBase->addResID = true;
        return urlBase;
    }
#endif
        
    urlBase->urlBase = Server::getInstance()->getVirtualURL() +
                       _(_URL_PARAM_SEPARATOR) +
                       CONTENT_MEDIA_HANDLER + _(_URL_PARAM_SEPARATOR) + 
                       dict->encodeSimple() + _(_URL_PARAM_SEPARATOR) + 
                       _(URL_RESOURCE_ID) + _(_URL_PARAM_SEPARATOR);
    urlBase->addResID = true;
    return urlBase;
}

String CdsResourceManager::getFirstResource(Ref<CdsItem> item)
{
    Ref<UrlBase> urlBase = addResources_getUrlBase(item);

    if (urlBase->addResID)
        return urlBase->urlBase + 0;
    else
        return urlBase->urlBase;
}
