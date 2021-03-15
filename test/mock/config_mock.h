#ifndef __CONFIG_MOCK_H__
#define __CONFIG_MOCK_H__

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "config/config.h"

using namespace ::testing;

class ConfigMock : public Config {
public:
    fs::path getConfigFilename() const override { return ""; }
    fs::path getDataDir() const override { return ""; }
    MOCK_METHOD(std::string, getOption, (config_option_t option), (const override));
    void addOption(config_option_t option, std::shared_ptr<ConfigOption> optionValue) override { }
    int getIntOption(config_option_t option) const override { return 0; }
    bool getBoolOption(config_option_t option) const override { return false; }
    std::map<std::string, std::string> getDictionaryOption(config_option_t option) const override
    {
        switch (option) {
            case CFG_UPNP_ALBUM_PROPERTIES:
                return {
                    { "dc:creator", "M_ALBUMARTIST" },
                    { "upnp:artist", "M_ALBUMARTIST" },
                    { "upnp:albumArtist", "M_ALBUMARTIST" },
                    { "upnp:composer", "M_COMPOSER" },
                    { "upnp:conductor", "M_CONDUCTOR" },
                    { "upnp:orchestra", "M_ORCHESTRA" },
                    { "upnp:date", "M_UPNP_DATE" },
                    { "dc:date", "M_UPNP_DATE" },
                    { "upnp:producer", "M_PRODUCER" },
                    { "dc:publisher", "M_PUBLISHER" },
                    { "upnp:genre", "M_GENRE" },
                };
            default:
                return std::map<std::string, std::string>();
        }
    }
    std::vector<std::string> getArrayOption(config_option_t option) const override { return std::vector<std::string>(); }
    std::shared_ptr<AutoscanList> getAutoscanListOption(config_option_t option) const override { return nullptr; }
    std::shared_ptr<ClientConfigList> getClientConfigListOption(config_option_t option) const override { return nullptr; }
    std::shared_ptr<DirectoryConfigList> getDirectoryTweakOption(config_option_t option) const override { return nullptr; }
    void updateConfigFromDatabase(std::shared_ptr<Database> database) override {};
    std::string getOrigValue(const std::string& item) const override { return ""; }
    void setOrigValue(const std::string& item, const std::string& value) override { }
    void setOrigValue(const std::string& item, bool value) override { }
    void setOrigValue(const std::string& item, int value) override { }
    bool hasOrigValue(const std::string& item) const override { return false; }
    MOCK_METHOD(std::shared_ptr<TranscodingProfileList>, getTranscodingProfileListOption, (config_option_t option), (const override));
};

#endif // __CONFIG_MOCK_H__
