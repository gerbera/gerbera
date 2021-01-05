/*GRB*

    Gerbera - https://gerbera.io/
    
    upnp_common.h - this file is part of Gerbera.
    
    Copyright (C) 2021 Gerbera Contributors
    
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
#define UPNP_DEFAULT_CLASS_CONTAINER "object.container"
#define UPNP_DEFAULT_CLASS_ITEM "object.item"
#define UPNP_DEFAULT_CLASS_VIDEO_ITEM "object.item.videoItem"
#define UPNP_DEFAULT_CLASS_IMAGE_ITEM "object.item.imageItem"
#define UPNP_DEFAULT_CLASS_MUSIC_ALBUM "object.container.album.musicAlbum"
#define UPNP_DEFAULT_CLASS_MUSIC_TRACK "object.item.audioItem.musicTrack"
#define UPNP_DEFAULT_CLASS_MUSIC_GENRE "object.container.genre.musicGenre"
#define UPNP_DEFAULT_CLASS_MUSIC_ARTIST "object.container.person.musicArtist"
#define UPNP_DEFAULT_CLASS_MUSIC_COMPOSER "object.container.person.musicComposer"
#define UPNP_DEFAULT_CLASS_MUSIC_CONDUCTOR "object.container.person.musicConductor"
#define UPNP_DEFAULT_CLASS_MUSIC_ORCHESTRA "object.container.person.musicOrchestra"
#define UPNP_DEFAULT_CLASS_PLAYLIST_CONTAINER "object.container.playlistContainer"
#define UPNP_DEFAULT_CLASS_VIDEO_BROADCAST "object.item.videoItem.videoBroadcast"

#define D_HTTP_TRANSFER_MODE_HEADER "transferMode.dlna.org"
#define D_HTTP_TRANSFER_MODE_STREAMING "Streaming"
#define D_HTTP_TRANSFER_MODE_INTERACTIVE "Interactive"
#define D_HTTP_CONTENT_FEATURES_HEADER "contentFeatures.dlna.org"

#define D_PROFILE "DLNA.ORG_PN"
#define D_CONVERSION_INDICATOR "DLNA.ORG_CI"
#define D_NO_CONVERSION "0"
#define D_CONVERSION "1"
#define D_OP "DLNA.ORG_OP"
// all seek modes
#define D_OP_SEEK_RANGE "01"
#define D_OP_SEEK_TIME "10"
#define D_OP_SEEK_BOTH "11"
#define D_OP_SEEK_DISABLED "00"
// since we are using only range seek
#define D_OP_SEEK_ENABLED D_OP_SEEK_RANGE
#define D_FLAGS "DLNA.ORG_FLAGS"
#define D_TR_FLAGS_AV "01200000000000000000000000000000"
#define D_TR_FLAGS_IMAGE "00800000000000000000000000000000"
#define D_MP3 "MP3"
#define D_LPCM "LPCM"
#define D_JPEG_SM "JPEG_SM"
#define D_JPEG_MED "JPEG_MED"
#define D_JPEG_LRG "JPEG_LRG"
#define D_JPEG_TN "JPEG_TN"
#define D_JPEG_SM_ICO "JPEG_SM_ICO"
#define D_JPEG_LRG_ICO "JPEG_LRG_ICO"
#define D_PN_MPEG_PS_PAL "MPEG_PS_PAL"
#define D_PN_MKV "MKV"
#define D_PN_AVC_MP4_EU "AVC_MP4_EU"
#define D_PN_AVI "AVI"

// device description defaults
#define DESC_DEVICE_NAMESPACE "urn:schemas-upnp-org:device-1-0"
#define DESC_DEVICE_TYPE "urn:schemas-upnp-org:device:MediaServer:1"
#define DESC_SPEC_VERSION_MAJOR "1"
#define DESC_SPEC_VERSION_MINOR "0"

//services
// connection manager
#define DESC_CM_SERVICE_TYPE "urn:schemas-upnp-org:service:ConnectionManager:1"
#define DESC_CM_SERVICE_ID "urn:upnp-org:serviceId:ConnectionManager"
#define DESC_CM_SCPD_URL "cm.xml"
#define DESC_CM_CONTROL_URL "/upnp/control/cm"
#define DESC_CM_EVENT_URL "/upnp/event/cm"

// content directory
#define DESC_CDS_SERVICE_TYPE "urn:schemas-upnp-org:service:ContentDirectory:1"
#define DESC_CDS_SERVICE_ID "urn:upnp-org:serviceId:ContentDirectory"
#define DESC_CDS_SCPD_URL "cds.xml"
#define DESC_CDS_CONTROL_URL "/upnp/control/cds"
#define DESC_CDS_EVENT_URL "/upnp/event/cds"

#define DESC_ICON_PNG_MIMETYPE "image/png"
#define DESC_ICON_BMP_MIMETYPE "image/x-ms-bmp"
#define DESC_ICON_JPG_MIMETYPE "image/jpeg"

// media receiver registrar (xbox 360)
#define DESC_MRREG_SERVICE_TYPE "urn:microsoft.com:service:X_MS_MediaReceiverRegistrar:1"
#define DESC_MRREG_SERVICE_ID "urn:microsoft.com:serviceId:X_MS_MediaReceiverRegistrar"
#define DESC_MRREG_SCPD_URL "mr_reg.xml"
#define DESC_MRREG_CONTROL_URL "/upnp/control/mr_reg"
#define DESC_MRREG_EVENT_URL "/upnp/event/mr_reg"

// xml namespaces
#define XML_NAMESPACE_ATTR "xmlns"
#define XML_DIDL_LITE_NAMESPACE "urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/"
#define XML_DC_NAMESPACE_ATTR "xmlns:dc"
#define XML_DC_NAMESPACE "http://purl.org/dc/elements/1.1/"
#define XML_UPNP_NAMESPACE_ATTR "xmlns:upnp"
#define XML_UPNP_NAMESPACE "urn:schemas-upnp-org:metadata-1-0/upnp/"
#define XML_SEC_NAMESPACE_ATTR "xmlns:sec"
#define XML_SEC_NAMESPACE "http://www.sec.co.kr/"

#endif // __UPNP_COMMON_H__
