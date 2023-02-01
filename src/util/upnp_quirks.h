/*GRB*

    Gerbera - https://gerbera.io/

    upnp_quirks.h - this file is part of Gerbera.

    Copyright (C) 2020-2023 Gerbera Contributors

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

/// \file upnp_quirks.h

#ifndef __UPNP_QUIRKS_H__
#define __UPNP_QUIRKS_H__

#include "upnp_xml.h"
#include <memory>
#include <pugixml.hpp>
#include <vector>

using QuirkFlags = std::uint32_t;

#define QUIRK_FLAG_NONE 0x00000000
#define QUIRK_FLAG_SAMSUNG 0x00000001
#define QUIRK_FLAG_SAMSUNG_BOOKMARK_SEC 0x00000002
#define QUIRK_FLAG_SAMSUNG_BOOKMARK_MSEC 0x00000004
#define QUIRK_FLAG_IRADIO 0x00000008
#define QUIRK_FLAG_SAMSUNG_FEATURES 0x00000010
#define QUIRK_FLAG_SAMSUNG_HIDE_DYNAMIC 0x00000020
#define QUIRK_FLAG_PV_SUBTITLES 0x00000040
#define QUIRK_FLAG_PANASONIC 0x00000080
#define QUIRK_FLAG_STRICTXML 0x00000100

// forward declaration
class ActionRequest;
class ContentManager;
class Context;
class CdsItem;
class CdsObject;
class GrbNet;
class Headers;
class UpnpXMLBuilder;
struct ClientInfo;

class Quirks {
public:
    Quirks(std::shared_ptr<UpnpXMLBuilder> xmlBuilder, const std::shared_ptr<ClientManager>& clientManager, const std::shared_ptr<GrbNet>& addr, const std::string& userAgent);

    // Look for subtitle file and returns its URL in CaptionInfo.sec response header.
    // To be more compliant with original Samsung server we should check for getCaptionInfo.sec: 1 request header.
    void addCaptionInfo(const std::shared_ptr<CdsItem>& item, Headers& headers);

    /** \brief Add Samsung specific bookmark information to the request's result.
     *
     * \param item const std::shared_ptr<CdsItem>& Item which will be played and stores the bookmark information.
     * \param result pugi::xml_node& Answer content.
     * \return void
     *
     */
    void restoreSamsungBookMarkedPosition(const std::shared_ptr<CdsItem>& item, pugi::xml_node& result, int offset = 10) const;

    /** \brief Stored bookmark information into the database
     *
     * \param request const std::unique_ptr<ActionRequest>& request sent by Samsung client, which holds the position information which should be stored
     * \return void
     *
     */
    void saveSamsungBookMarkedPosition(const std::shared_ptr<Database>& database, ActionRequest& request) const;

    /** \brief get Samsung Feature List
     *
     * \param request const std::unique_ptr<ActionRequest>& request sent by Samsung client
     * \return void
     *
     */
    void getSamsungFeatureList(ActionRequest& request) const;
    std::vector<std::shared_ptr<CdsObject>> getSamsungFeatureRoot(const std::shared_ptr<Database>& database, const std::string& objId) const;

    /** \brief get Samsung ObjectID from Index
     *
     * \param request const std::unique_ptr<ActionRequest>& request sent by Samsung client
     * \return void
     *
     */
    void getSamsungObjectIDfromIndex(ActionRequest& request) const;

    /** \brief get Samsung Index from RID
     *
     * \param request const std::unique_ptr<ActionRequest>& request sent by Samsung client
     * \return void
     *
     */
    void getSamsungIndexfromRID(ActionRequest& request) const;

    /** \brief block XML header in response for broken clients
     *
     * \return bool
     *
     */
    bool blockXmlDeclaration() const;

    /** \brief client need the filename in the uri to determine language or other detailes
     *
     * \return bool
     *
     */
    bool needsFileNameUri() const;

    /** \brief Check whether the supplied flags are set
     *
     * \param bitset of the flags to check
     * \return bitset of the flags
     *
     */
    long checkFlags(long flags) const;

    /** \brief Get number of allow CaptionInfoEx entries
     *
     * \return number for client
     *
     */
    int getCaptionInfoCount() const;

    /** \brief Get max length of Upnp string
     *
     * \return string limit for client
     *
     */
    int getStringLimit() const;

    /** \brief UPnP client needs everything escaped, esp. '
     *
     * \return bool
     *
     */
    bool needsStrictXml() const;

    /** \brief Get multi value upnp properties
     *
     * \return true if multi-value is enabled for client
     *
     */
    bool getMultiValue() const;

    /** \brief Get group for ClientStatusDetail
     *
     * \return group for ClientStatusDetail
     *
     */
    std::string getGroup() const;

    /** \brief Get mime type mappings for client
     *
     * \return mime type mappings
     *
     */
    std::map<std::string, std::string> getMimeMappings() const;

private:
    std::shared_ptr<UpnpXMLBuilder> xmlBuilder;
    const ClientInfo* pClientInfo;

    bool hasFlag(long flag) const;
};

#endif // __UPNP_QUIRKS_H__
