/*GRB*
    Gerbera - https://gerbera.io/

    config_mock.h - this file is part of Gerbera.

    Copyright (C) 2016-2025 Gerbera Contributors

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
#ifndef __CONFIG_MOCK_H__
#define __CONFIG_MOCK_H__

#include "config/config.h"
#include "config/config_val.h"
#include "metadata/metadata_enums.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

class ConfigMock : public Config {
public:
    fs::path getConfigFilename() const override { return {}; }
    MOCK_METHOD(std::string, getOption, (ConfigVal option), (const override));
    void addOption(ConfigVal option, const std::shared_ptr<ConfigOption>& optionValue) override { }
    std::int32_t getIntOption(ConfigVal option) const override {
        switch (option) {
        case ConfigVal::UPNP_CAPTION_COUNT:
            return 1;
        default:
            return 0;
        }
    }
    std::uint32_t getUIntOption(ConfigVal option) const override { return 0; }
    long long getLongOption(ConfigVal option) const override { return 0; }
    unsigned long long getULongOption(ConfigVal option) const override { return 0; }
    std::shared_ptr<ConfigOption> getConfigOption(ConfigVal option) const override { return {}; }
    bool getBoolOption(ConfigVal option) const override
    {
        switch (option) {
#ifdef HAVE_FFMPEGTHUMBNAILER
        case ConfigVal::SERVER_EXTOPTS_FFMPEGTHUMBNAILER_ENABLED:
            return true;
#endif
        default:
            return false;
        }
    }
    std::string generateUDN(const std::shared_ptr<Database>& database) override { return "uuid:12345678-1234-1234-1234-123456789abc"; };
    std::map<std::string, std::string> getDictionaryOption(ConfigVal option) const override
    {
        switch (option) {
        case ConfigVal::IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST:
            return {
                { "application/ogg", CONTENT_TYPE_OGG },
                { "audio/L16", CONTENT_TYPE_PCM },
                { "audio/flac", CONTENT_TYPE_FLAC },
                { "audio/mp4", CONTENT_TYPE_MP4 },
                { "audio/mpeg", CONTENT_TYPE_MP3 },
                { "audio/ogg", CONTENT_TYPE_OGG },
                { "audio/x-dsd", CONTENT_TYPE_DSD },
                { "audio/x-flac", CONTENT_TYPE_FLAC },
                { "audio/x-matroska", CONTENT_TYPE_MKA },
                { "audio/x-mpegurl", CONTENT_TYPE_PLAYLIST },
                { "audio/x-ms-wma", CONTENT_TYPE_WMA },
                { "audio/x-scpls", CONTENT_TYPE_PLAYLIST },
                { "audio/x-wav", CONTENT_TYPE_PCM },
                { "audio/x-wavpack", CONTENT_TYPE_WAVPACK },
                { "image/jpeg", CONTENT_TYPE_JPG },
                { "image/png", CONTENT_TYPE_PNG },
                { "video/mkv", CONTENT_TYPE_MKV },
                { "video/mp4", CONTENT_TYPE_MP4 },
                { "video/mpeg", CONTENT_TYPE_MPEG },
                { "video/x-matroska", CONTENT_TYPE_MKV },
                { "video/x-mkv", CONTENT_TYPE_MKV },
                { "video/x-ms-asf", CONTENT_TYPE_ASF },
                { MIME_TYPE_ASX_PLAYLIST, CONTENT_TYPE_PLAYLIST },
                { "video/x-msvideo", CONTENT_TYPE_AVI },
            };
        case ConfigVal::UPNP_ALBUM_PROPERTIES:
            return {
                { "dc:creator", "M_ALBUMARTIST" },
                { "upnp:artist", "M_ALBUMARTIST" },
                { "upnp:albumArtist", "M_ALBUMARTIST" },
                { "upnp:composer", "M_COMPOSER" },
                { "upnp:conductor", "M_CONDUCTOR" },
                { "upnp:orchestra", "M_ORCHESTRA" },
                { "upnp:date", "M_UPNP_DATE" },
                { "dc:date", "M_DATE" },
                { "upnp:producer", "M_PRODUCER" },
                { "dc:publisher", "M_PUBLISHER" },
                { "upnp:genre", "M_GENRE" },
            };
        default:
            return {};
        }
    }
    std::vector<std::vector<std::pair<std::string, std::string>>> getVectorOption(ConfigVal option) const override
    {
        switch (option) {

        case ConfigVal::IMPORT_MAPPINGS_CONTENTTYPE_TO_DLNAPROFILE_LIST:
            return {
                { { "from", CONTENT_TYPE_ASF }, { "to", "VC_ASF_AP_L2_WMA" } },
                { { "from", CONTENT_TYPE_AVI }, { "to", "AVI" } },
                { { "from", CONTENT_TYPE_DSD }, { "to", "DSF" } },
                { { "from", CONTENT_TYPE_FLAC }, { "to", "FLAC" } },
                { { "from", CONTENT_TYPE_JPG }, { "to", "JPEG_LRG" } },
                { { "from", CONTENT_TYPE_MKA }, { "to", "MKV" } },
                { { "from", CONTENT_TYPE_MKV }, { "to", "MKV" } },
                { { "from", CONTENT_TYPE_MP3 }, { "to", "MP3" } },
                { { "from", CONTENT_TYPE_MP4 }, { "to", "AVC_MP4_EU" } },
                { { "from", CONTENT_TYPE_MPEG }, { "to", "MPEG_PS_PAL" } },
                { { "from", CONTENT_TYPE_OGG }, { "to", "OGG" } },
                { { "from", CONTENT_TYPE_PCM }, { "to", "LPCM" } },
                { { "from", CONTENT_TYPE_PNG }, { "to", "PNG_LRG" } },
                { { "from", CONTENT_TYPE_WMA }, { "to", "WMAFULL" } },
            };
        default:
            return {};
        }
    }
    std::vector<std::string> getArrayOption(ConfigVal option) const override { return {}; }
    std::vector<std::shared_ptr<AutoscanDirectory>> getAutoscanListOption(ConfigVal option) const override { return {}; }
    std::shared_ptr<BoxLayoutList> getBoxLayoutListOption(ConfigVal option) const override { return {}; }
    std::shared_ptr<ClientConfigList> getClientConfigListOption(ConfigVal option) const override { return nullptr; }
    std::shared_ptr<DirectoryConfigList> getDirectoryTweakOption(ConfigVal option) const override { return nullptr; }
    void updateConfigFromDatabase(const std::shared_ptr<Database>& database) override { }
    std::string getOrigValue(const std::string& item) const override { return {}; }
    void setOrigValue(const std::string& item, const std::string& value) override { }
    void setOrigValue(const std::string& item, bool value) override { }
    void setOrigValue(const std::string& item, std::int32_t value) override { }
    void setOrigValue(const std::string& item, std::uint32_t value) override { }
    void setOrigValue(const std::string& item, long long value) override { }
    void setOrigValue(const std::string& item, unsigned long long value) override { }
    bool hasOrigValue(const std::string& item) const override { return false; }
    MOCK_METHOD(std::shared_ptr<TranscodingProfileList>, getTranscodingProfileListOption, (ConfigVal option), (const override));
    MOCK_METHOD(std::shared_ptr<DynamicContentList>, getDynamicContentListOption, (ConfigVal option), (const override));
    void registerNode(const std::string& xmlPath) override {};
};

#endif // __CONFIG_MOCK_H__
