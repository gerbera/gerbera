/*GRB*

    Gerbera - https://gerbera.io/

    upnp_common.h - this file is part of Gerbera.

    Copyright (C) 2021-2024 Gerbera Contributors

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

/// \file upnp_common.h
#ifndef __UPNP_COMMON_H__
#define __UPNP_COMMON_H__

// ERROR CODES
/// \brief UPnP specific error code.
#define UPNP_E_ACTION_FAILED 501
#define UPNP_E_SUBSCRIPTION_FAILED 503
/// \brief UPnP specific error code.
#define UPNP_E_NO_SUCH_ID 701
#define UPNP_E_NOT_EXIST 706

// UPnP default classes
#define UPNP_CLASS_ITEM "object.item"
#define UPNP_CLASS_PLAYLIST_ITEM "object.item.playlistItem"
#define UPNP_SEARCH_CLASS "upnp:class"
#define UPNP_SEARCH_ARTIST "upnp:artist"
#define UPNP_SEARCH_GENRE "upnp:genre"
#define UPNP_SEARCH_LAST_PLAYED "upnp:lastPlaybackTime"
#define UPNP_SEARCH_PLAY_COUNT "upnp:playbackCount"

#define UPNP_CLASS_AUDIO_ITEM "object.item.audioItem"
#define UPNP_CLASS_MUSIC_TRACK "object.item.audioItem.musicTrack"
#define UPNP_CLASS_AUDIO_BOOK "object.item.audioItem.audioBook"
#define UPNP_CLASS_AUDIO_BROADCAST "object.item.audioItem.audioBroadcast"

#define UPNP_CLASS_IMAGE_ITEM "object.item.imageItem"
#define UPNP_CLASS_IMAGE_PHOTO "object.item.imageItem.photo"

#define UPNP_CLASS_VIDEO_ITEM "object.item.videoItem"
#define UPNP_CLASS_VIDEO_MOVIE "object.item.videoItem.movie"
#define UPNP_CLASS_VIDEO_MUSICVIDEOCLIP "object.item.videoItem.musicVideoClip"
#define UPNP_CLASS_VIDEO_BROADCAST "object.item.videoItem.videoBroadcast"

#define UPNP_CLASS_TEXT_ITEM "object.item.textItem"
#define UPNP_CLASS_BOOKMARK_ITEM "object.item.bookmarkItem"

#define UPNP_CLASS_CONTAINER "object.container"
#define UPNP_CLASS_CONTAINER_FOLDER "object.container.storageFolder"
#define UPNP_CLASS_PHOTO_ALBUM "object.container.album.photoAlbum"
#define UPNP_CLASS_MUSIC_ALBUM "object.container.album.musicAlbum"
#define UPNP_CLASS_MUSIC_GENRE "object.container.genre.musicGenre"
#define UPNP_CLASS_MUSIC_ARTIST "object.container.person.musicArtist"
#define UPNP_CLASS_MUSIC_COMPOSER "object.container.person.musicComposer"
#define UPNP_CLASS_MUSIC_CONDUCTOR "object.container.person.musicConductor"
#define UPNP_CLASS_MUSIC_ORCHESTRA "object.container.person.musicOrchestra"
#define UPNP_CLASS_PLAYLIST_CONTAINER "object.container.playlistContainer"

#define UPNP_CLASS_DYNAMIC_CONTAINER "object.container.dynamicFolder"

// transferMode
#define UPNP_DLNA_TRANSFER_MODE_HEADER "transferMode.dlna.org"
#define UPNP_DLNA_TRANSFER_MODE_STREAMING "Streaming"
#define UPNP_DLNA_TRANSFER_MODE_INTERACTIVE "Interactive"
#define UPNP_DLNA_TRANSFER_MODE_BACKGROUND "Background"

// contentFeatures
#define UPNP_DLNA_CONTENT_FEATURES_HEADER "contentFeatures.dlna.org"

#define UPNP_DESC_STYLESHEET "/css/upnp.css"

// device description defaults
#define UPNP_DESC_SCPD_URL "/upnp"
#define UPNP_DESC_DEVICE_DESCRIPTION "/description.xml"

// services
//  connection manager
#define UPNP_DESC_CM_SERVICE_TYPE "urn:schemas-upnp-org:service:ConnectionManager:1"
#define UPNP_DESC_CM_SERVICE_ID "urn:upnp-org:serviceId:ConnectionManager"
#define UPNP_DESC_CM_SCPD_URL "/cm.xml"
#define UPNP_DESC_CM_CONTROL_URL "/upnp/control/cm"
#define UPNP_DESC_CM_EVENT_URL "/upnp/event/cm"

// content directory
#define UPNP_DESC_CDS_SERVICE_TYPE "urn:schemas-upnp-org:service:ContentDirectory:1"
#define UPNP_DESC_CDS_SERVICE_ID "urn:upnp-org:serviceId:ContentDirectory"
#define UPNP_DESC_CDS_SCPD_URL "/cds.xml"
#define UPNP_DESC_CDS_CONTROL_URL "/upnp/control/cds"
#define UPNP_DESC_CDS_EVENT_URL "/upnp/event/cds"

#define UPNP_DESC_ICON_PNG_MIMETYPE "image/png"
#define UPNP_DESC_ICON_BMP_MIMETYPE "image/x-ms-bmp"
#define UPNP_DESC_ICON_JPG_MIMETYPE "image/jpeg"

// media receiver registrar (xbox 360)
#define UPNP_DESC_MRREG_SERVICE_TYPE "urn:microsoft.com:service:X_MS_MediaReceiverRegistrar:1"
#define UPNP_DESC_MRREG_SERVICE_ID "urn:microsoft.com:serviceId:X_MS_MediaReceiverRegistrar"
#define UPNP_DESC_MRREG_SCPD_URL "/mr_reg.xml"
#define UPNP_DESC_MRREG_CONTROL_URL "/upnp/control/mr_reg"
#define UPNP_DESC_MRREG_EVENT_URL "/upnp/event/mr_reg"

// xml namespaces
#define UPNP_XML_DLNA_NAMESPACE_ATTR "xmlns:dlna"
#define UPNP_XML_DLNA_NAMESPACE "urn:schemas-dlna-org:device-1-0"
#define UPNP_XML_DLNA_METADATA_NAMESPACE "urn:schemas-dlna-org:metadata-1-0"
#define UPNP_XML_DIDL_LITE_NAMESPACE_ATTR "xmlns"
#define UPNP_XML_DIDL_LITE_NAMESPACE "urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/"
#define UPNP_XML_DC_NAMESPACE_ATTR "xmlns:dc"
#define UPNP_XML_DC_NAMESPACE "http://purl.org/dc/elements/1.1/"
#define UPNP_XML_UPNP_NAMESPACE_ATTR "xmlns:upnp"
#define UPNP_XML_UPNP_NAMESPACE "urn:schemas-upnp-org:metadata-1-0/upnp/"
#define UPNP_XML_SEC_NAMESPACE_ATTR "xmlns:sec"
#define UPNP_XML_SEC_NAMESPACE "http://www.sec.co.kr/dlna"

#endif // __UPNP_COMMON_H__
