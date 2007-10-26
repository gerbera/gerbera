/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    cds_resource_manager.cc - this file is part of MediaTomb.
    
    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>
    
    Copyright (C) 2006-2007 Gena Batyan <bgeradz@mediatomb.cc>,
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

#ifdef HAVE_CONFIG_H
#include "autoconfig.h"
#endif

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
    String ext = String(_URL_ARG_SEPARATOR) + _(URL_FILE_EXTENSION) + _("=");

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
               (extension.index(URL_ARG_SEPARATOR) == -1))
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

    Ref<Dictionary> mappings = config->getDictionaryOption(
                        CFG_IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST);


#ifdef TRANSCODING
    // this will be used to count only the "real" resources, omitting the
    // transcoded ones
    int realCount = 0;
    bool hide_original_resource = false;
    
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

            if (mappings->get(item->getMimeType()) == CONTENT_TYPE_OGG) 
            {
                if (((item->getFlag(OBJECT_FLAG_OGG_THEORA)) && 
                     (!tp->isTheora())) ||
                    (!item->getFlag(OBJECT_FLAG_OGG_THEORA) && 
                    (tp->isTheora())))
                     {
                         continue;
                     }
            }

            Ref<CdsResource> t_res(new CdsResource(CH_TRANSCODE));
            t_res->addParameter(_(URL_PARAM_TRANSCODE_PROFILE_NAME), tp->getName());
            // after transcoding resource was added we can not rely on
            // index 0, so we will make sure the ogg option is there
            t_res->addOption(_(CONTENT_TYPE_OGG), 
                         item->getResource(0)->getOption(_(CONTENT_TYPE_OGG)));
            t_res->addParameter(_(URL_PARAM_TRANSCODE), _(D_CONVERSION));
            t_res->addAttribute(MetadataHandler::getResAttrName(R_PROTOCOLINFO),
                    renderProtocolInfo(tp->getTargetMimeType()));
            // duration should be the same for transcoded media, so we can take
            // the value from the original resource
            String duration = item->getResource(0)->getAttribute(MetadataHandler::getResAttrName(R_DURATION));
            if (string_ok(duration))
                t_res->addAttribute(MetadataHandler::getResAttrName(R_DURATION),
                        duration);

            t_res->mergeAttributes(tp->getAttributes());

            if (tp->hideOriginalResource())
                hide_original_resource = true;

            if (tp->firstResource())
                item->insertResource(0, t_res);
            else
                item->addResource(t_res);
        }

        if (skipURL)
            urlBase_tr = addResources_getUrlBase(item, true);
    }

#endif

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
        // the for transcoded resources the res_id can be safely ignored,
        // because a transcoded resource is identified by the profile name
#ifdef TRANSCODING
        // flag if we are dealing with the transcoded resource
        bool transcoded = (res_params->get(_(URL_PARAM_TRANSCODE)) == D_CONVERSION);
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
            url = url + _(_URL_ARG_SEPARATOR);
            url = url + res_params->encode();
        }

        /// \todo currently resource is misused for album art
#ifdef HAVE_ID3_ALBUMART
        // only add upnp:AlbumArtURI if we have an AA, skip the resource
        if ((i > 0) && (item->getResource(i)->getHandlerType() == CH_ID3))
        {
            String rct = item->getResource(i)->getParameter(_(RESOURCE_CONTENT_TYPE));
            if (rct == ID3_ALBUM_ART)
            {
                element->appendTextChild(
                        MetadataHandler::getMetaFieldName(M_ALBUMARTURI),
                        url);
            }
            continue;
        }
#endif

#ifdef TRANSCODING

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

#ifdef EXTEND_PROTOCOLINFO
        if (config->getBoolOption(CFG_SERVER_EXTEND_PROTOCOLINFO))
        {
            String extend;
            if (contentType == CONTENT_TYPE_MP3)
                extend = _(D_PROFILE) + "=" + D_MP3 + ";";
            else if (contentType == CONTENT_TYPE_PCM)
                extend = _(D_PROFILE) + "=" + D_LPCM + ";";
       

#ifdef TRANSCODING
        // we do not support seeking at all, so 00
        // and the media is converted, so set CI to 1
        if (transcoded)
            extend = extend + D_OP + "=00;" + 
                     D_CONVERSION_INDICATOR + "=" D_CONVERSION;
        else
#endif
        extend = extend + D_OP + "=01;" + 
                 D_CONVERSION_INDICATOR + "=" + D_NO_CONVERSION;

        protocolInfo = protocolInfo.substring(0, protocolInfo.rindex(':')+1) + 
                       extend;
        res_attrs->put(MetadataHandler::getResAttrName(R_PROTOCOLINFO),
                       protocolInfo);

        log_debug("extended protocolInfo: %s\n", protocolInfo.c_str());
        }
#endif
#ifdef TRANSCODING
        if (!hide_original_resource || transcoded)
#endif
            element->appendChild(UpnpXML_DIDLRenderResource(url, res_attrs));
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
        urlBase->urlBase = Server::getInstance()->getVirtualURL() + _("/") + 
                           CONTENT_SERVE_HANDLER + 
                           _("/") + item->getLocation();
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
            urlBase->urlBase = Server::getInstance()->getVirtualURL() + _("/") +
                CONTENT_ONLINE_HANDLER + _(_URL_PARAM_SEPARATOR) +
                dict->encode() + _(_URL_ARG_SEPARATOR) +
                _(URL_RESOURCE_ID) + _("=");
            urlBase->addResID = true;
            return urlBase;
        }
    }
        
    urlBase->urlBase = Server::getInstance()->getVirtualURL() + _("/") +
                       CONTENT_MEDIA_HANDLER + _(_URL_PARAM_SEPARATOR) + 
                       dict->encode() + _(_URL_ARG_SEPARATOR) + 
                       _(URL_RESOURCE_ID) + _("=");
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

