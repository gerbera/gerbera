/*GRB*

    Gerbera - https://gerbera.io/

    quirks.h - this file is part of Gerbera.

    Copyright (C) 2020-2025 Gerbera Contributors

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

/// @file upnp/quirks.h

#ifndef __UPNP_QUIRKS_H__
#define __UPNP_QUIRKS_H__

#include <cstdint>
#include <map>
#include <memory>
#include <vector>

namespace pugi {
class xml_node;
} // namespace pugi

using QuirkFlags = std::uint32_t;

static constexpr QuirkFlags QUIRK_FLAG_NONE = 0x00000000;

// for documentaion see supported-devices.rst
enum class Quirk {
    None = -1,
    Samsung = 0,
    SamsungBookmarkSeconds,
    SamsungBookmarkMilliSeconds,
    NoXmlDeclaration,
    SamsungFeatures,
    SamsungHideDynamic,
    PvSubtitles,
    StrictXML,
    HideResourceThumbnail,
    HideResourceSubtitle,
    HideResourceTranscode,
    SimpleDate,
    DCM10,
    HideContainerShortcuts,
    AsciiXML,
    ForceNoConversion,
    ShowInternalSubtitles,
    ForceSortCriteriaTitle,
    CaptionProtocol,
    Transcoding1,
    Transcoding2,
    Transcoding3,
};

// forward declaration
class ActionRequest;
class ClientManager;
class Context;
class CdsItem;
class CdsObject;
class Database;
class GrbNet;
class Headers;
class UpnpXMLBuilder;
enum class ResourcePurpose;
struct ClientProfile;
struct ClientObservation;

/// @brief class to handle client specific behaviour and details
class Quirks {
public:
    Quirks(
        std::shared_ptr<UpnpXMLBuilder> xmlBuilder,
        const std::shared_ptr<ClientManager>& clientManager,
        const std::shared_ptr<GrbNet>& addr,
        const std::string& userAgent,
        const std::shared_ptr<Headers>& headers);

    Quirks(const ClientObservation* client);

    // Look for subtitle file and returns its URL in CaptionInfo.sec response header.
    // To be more compliant with original Samsung server we should check for getCaptionInfo.sec: 1 request header.
    void addCaptionInfo(const std::shared_ptr<CdsItem>& item, Headers& headers);

    /** @brief Add Samsung specific bookmark information to the request's result.
     *
     * @param item CdsItem which will be played and stores the bookmark information.
     * @param result Answer content.
     * @param offset number of seconds to jump
     *
     */
    void restoreSamsungBookMarkedPosition(
        const std::shared_ptr<CdsItem>& item,
        pugi::xml_node& result,
        int offset = 10) const;

    /** @brief Stored bookmark information into the database
     *
     * @param database storage to retrieve position from
     * @param request request sent by Samsung client, which holds the position information which should be stored
     * @return void
     *
     */
    void saveSamsungBookMarkedPosition(const std::shared_ptr<Database>& database, ActionRequest& request) const;

    /** @brief get UPnP shortcut List
     *
     * @param database DB interface
     * @param features pugi::xml_node item in response
     * @return void
     *
     */
    void getShortCutList(const std::shared_ptr<Database>& database, pugi::xml_node& features) const;

    /** @brief get Samsung Feature List
     *
     * @param request const std::unique_ptr<ActionRequest>& request sent by Samsung client
     * @return void
     *
     */
    void getSamsungFeatureList(ActionRequest& request) const;
    /// @brief find container matching the objId
    /// @param database to search in
    /// @param startIndex of search results
    /// @param count number of search results
    /// @param objId to search root container for
    /// @return list of CdsObject matching the content class
    std::vector<std::shared_ptr<CdsObject>> getSamsungFeatureRoot(
        const std::shared_ptr<Database>& database,
        int startIndex,
        int count,
        const std::string& objId) const;

    /** @brief get Samsung ObjectID from Index
     *
     * @param request const std::unique_ptr<ActionRequest>& request sent by Samsung client
     * @return void
     *
     */
    void getSamsungObjectIDfromIndex(ActionRequest& request) const;

    /** @brief get Samsung Index from RID
     *
     * @param request const std::unique_ptr<ActionRequest>& request sent by Samsung client
     * @return void
     *
     */
    void getSamsungIndexfromRID(ActionRequest& request) const;

    /** @brief Check whether clients support resource type
     *
     * @return bool
     *
     */
    bool supportsResource(ResourcePurpose purpose) const;

    /** @brief Get number of allow CaptionInfoEx entries
     *
     * @return number for client
     *
     */
    int getCaptionInfoCount() const;

    /** @brief Get max length of Upnp string
     *
     * @return string limit for client
     *
     */
    int getStringLimit() const;

    /** @brief Get multi value upnp properties
     *
     * @return true if multi-value is enabled for client
     *
     */
    bool getMultiValue() const;

    /** @brief Get full upnp filter creation
     *
     * @return true if full-filter is enabled for client
     *
     */
    bool getFullFilter() const;

    /** @brief Get group for ClientStatusDetail
     *
     * @return group for ClientStatusDetail
     *
     */
    std::string getGroup() const;

    /** @brief Get mime type mappings for client
     *
     * @return mime type mappings
     *
     */
    std::map<std::string, std::string> getMimeMappings() const;

    /** @brief Get dlna profile mappings for client
     *
     * @return dlna profile mappings
     *
     */
    std::vector<std::vector<std::pair<std::string, std::string>>> getDlnaMappings() const;

    /** @brief Update Headers with overwrites for client
     */
    void updateHeaders(Headers& headers) const;

    /** @brief Client may connect to server
     */
    bool isAllowed() const;

    /** @brief Check for active flag
     */
    bool hasFlag(QuirkFlags flag) const;
    bool hasFlag(Quirk flag) const;

    /** @brief Check for header entry
     */
    bool hasHeader(const std::string& key, const std::string& value) const;

    /** @brief Get list of source folders to hide from client
     */
    std::vector<std::string> getForbiddenDirectories() const;

    const ClientProfile* getProfile() const { return pClientProfile; }
    const ClientObservation* getClient() const { return pClient; }

private:
    std::shared_ptr<UpnpXMLBuilder> xmlBuilder;
    const ClientProfile* pClientProfile;
    const ClientObservation* pClient;
};

#endif // __UPNP_QUIRKS_H__
