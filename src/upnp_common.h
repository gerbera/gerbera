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
#define UPNP_CLASS_CONTAINER "object.container"
#define UPNP_CLASS_ITEM "object.item"
#define UPNP_CLASS_VIDEO_ITEM "object.item.videoItem"
#define UPNP_CLASS_IMAGE_ITEM "object.item.imageItem"
#define UPNP_CLASS_MUSIC_ALBUM "object.container.album.musicAlbum"
#define UPNP_CLASS_MUSIC_TRACK "object.item.audioItem.musicTrack"
#define UPNP_CLASS_MUSIC_GENRE "object.container.genre.musicGenre"
#define UPNP_CLASS_MUSIC_ARTIST "object.container.person.musicArtist"
#define UPNP_CLASS_MUSIC_COMPOSER "object.container.person.musicComposer"
#define UPNP_CLASS_MUSIC_CONDUCTOR "object.container.person.musicConductor"
#define UPNP_CLASS_MUSIC_ORCHESTRA "object.container.person.musicOrchestra"
#define UPNP_CLASS_PLAYLIST_CONTAINER "object.container.playlistContainer"
#define UPNP_CLASS_VIDEO_BROADCAST "object.item.videoItem.videoBroadcast"

// transferMode
#define UPNP_DLNA_TRANSFER_MODE_HEADER "transferMode.dlna.org"
#define UPNP_DLNA_TRANSFER_MODE_STREAMING "Streaming"
#define UPNP_DLNA_TRANSFER_MODE_INTERACTIVE "Interactive"

// contentFeatures
#define UPNP_DLNA_CONTENT_FEATURES_HEADER "contentFeatures.dlna.org"
#define UPNP_DLNA_PROFILE "DLNA.ORG_PN"
#define UPNP_DLNA_CONVERSION_INDICATOR "DLNA.ORG_CI"
#define UPNP_DLNA_OP "DLNA.ORG_OP"
#define UPNP_DLNA_FLAGS "DLNA.ORG_FLAGS"

// DLNA.ORG_CI flags
//
// 0 - media is not transcoded
// 1 - media is transcoded
#define UPNP_DLNA_NO_CONVERSION "0"
#define UPNP_DLNA_CONVERSION "1"

// DLNA.ORG_OP flags
//
// Two booleans (binary digits) which determine what transport operations the renderer is allowed to
// perform (in the form of HTTP request headers): the first digit allows the renderer to send
// TimeSeekRange.DLNA.ORG (seek by time) headers; the second allows it to send RANGE (seek by byte)
// headers.
//
// 0x00 - no seeking (or even pausing) allowed
// 0x01 - seek by byte (exclusive)
// 0x10 - seek by time (exclusive)
// 0x11 - seek by both
#define UPNP_DLNA_OP_SEEK_DISABLED "00"
#define UPNP_DLNA_OP_SEEK_RANGE "01"
#define UPNP_DLNA_OP_SEEK_TIME "10"
#define UPNP_DLNA_OP_SEEK_BOTH "11"

// DLNA.ORG_FLAGS flags
//
// 0x00100000 - dlna V1.5
// 0x00200000 - connection stalling
// 0x00400000 - background transfer mode
// 0x00800000 - interactive transfer mode
// 0x01000000 - streaming transfer mode
#define UPNP_DLNA_ORG_FLAGS_AV "01700000000000000000000000000000"
#define UPNP_DLNA_ORG_FLAGS_IMAGE "00800000000000000000000000000000"

// DLNA.ORG_PN flags
#define UPNP_DLNA_PROFILE_MKV "MKV"
#define UPNP_DLNA_PROFILE_AVC_MP4_EU "AVC_MP4_EU"
#define UPNP_DLNA_PROFILE_AVI "AVI"
#define UPNP_DLNA_PROFILE_MPEG_PS_PAL "MPEG_PS_PAL"
#define UPNP_DLNA_PROFILE_MP3 "MP3"
#define UPNP_DLNA_PROFILE_LPCM "LPCM"
#define UPNP_DLNA_PROFILE_JPEG_SM "JPEG_SM"
#define UPNP_DLNA_PROFILE_JPEG_MED "JPEG_MED"
#define UPNP_DLNA_PROFILE_JPEG_LRG "JPEG_LRG"
#define UPNP_DLNA_PROFILE_JPEG_TN "JPEG_TN"
//#define UPNP_DLNA_PROFILE_JPEG_SM_ICO "JPEG_SM_ICO"
//#define UPNP_DLNA_PROFILE_JPEG_LRG_ICO "JPEG_LRG_ICO"

// device description defaults
#define UPNP_DESC_DEVICE_NAMESPACE "urn:schemas-upnp-org:device-1-0"
#define UPNP_DESC_DEVICE_TYPE "urn:schemas-upnp-org:device:MediaServer:1"
#define UPNP_DESC_SPEC_VERSION_MAJOR "1"
#define UPNP_DESC_SPEC_VERSION_MINOR "0"

//services
// connection manager
#define UPNP_DESC_CM_SERVICE_TYPE "urn:schemas-upnp-org:service:ConnectionManager:1"
#define UPNP_DESC_CM_SERVICE_ID "urn:upnp-org:serviceId:ConnectionManager"
#define UPNP_DESC_CM_SCPD_URL "cm.xml"
#define UPNP_DESC_CM_CONTROL_URL "/upnp/control/cm"
#define UPNP_DESC_CM_EVENT_URL "/upnp/event/cm"

// content directory
#define UPNP_DESC_CDS_SERVICE_TYPE "urn:schemas-upnp-org:service:ContentDirectory:1"
#define UPNP_DESC_CDS_SERVICE_ID "urn:upnp-org:serviceId:ContentDirectory"
#define UPNP_DESC_CDS_SCPD_URL "cds.xml"
#define UPNP_DESC_CDS_CONTROL_URL "/upnp/control/cds"
#define UPNP_DESC_CDS_EVENT_URL "/upnp/event/cds"

#define UPNP_DESC_ICON_PNG_MIMETYPE "image/png"
#define UPNP_DESC_ICON_BMP_MIMETYPE "image/x-ms-bmp"
#define UPNP_DESC_ICON_JPG_MIMETYPE "image/jpeg"

// media receiver registrar (xbox 360)
#define UPNP_DESC_MRREG_SERVICE_TYPE "urn:microsoft.com:service:X_MS_MediaReceiverRegistrar:1"
#define UPNP_DESC_MRREG_SERVICE_ID "urn:microsoft.com:serviceId:X_MS_MediaReceiverRegistrar"
#define UPNP_DESC_MRREG_SCPD_URL "mr_reg.xml"
#define UPNP_DESC_MRREG_CONTROL_URL "/upnp/control/mr_reg"
#define UPNP_DESC_MRREG_EVENT_URL "/upnp/event/mr_reg"

// xml namespaces
#define UPNP_XML_DIDL_LITE_NAMESPACE_ATTR "xmlns"
#define UPNP_XML_DIDL_LITE_NAMESPACE "urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/"
#define UPNP_XML_DC_NAMESPACE_ATTR "xmlns:dc"
#define UPNP_XML_DC_NAMESPACE "http://purl.org/dc/elements/1.1/"
#define UPNP_XML_UPNP_NAMESPACE_ATTR "xmlns:upnp"
#define UPNP_XML_UPNP_NAMESPACE "urn:schemas-upnp-org:metadata-1-0/upnp/"
#define UPNP_XML_SEC_NAMESPACE_ATTR "xmlns:sec"
#define UPNP_XML_SEC_NAMESPACE "http://www.sec.co.kr/dlna"

#endif // __UPNP_COMMON_H__
