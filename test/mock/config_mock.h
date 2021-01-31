#ifndef __CONFIG_MOCK_H__
#define __CONFIG_MOCK_H__

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "config/config.h"

using namespace ::testing;

class ConfigMock : public Config {
public:
    fs::path getConfigFilename() const override { return ""; }
    MOCK_METHOD(std::string, getOption, (config_option_t option), (const override));
    void addOption(config_option_t option, std::shared_ptr<ConfigOption> optionValue) override { }
    int getIntOption(config_option_t option) const override { return 0; }
    bool getBoolOption(config_option_t option) const override { return false; }
    std::map<std::string, std::string> getDictionaryOption(config_option_t option) const override { return std::map<std::string, std::string>(); }
    std::vector<std::string> getArrayOption(config_option_t option) const override { return std::vector<std::string>(); }
    std::shared_ptr<AutoscanList> getAutoscanListOption(config_option_t option) const override { return nullptr; }
    std::shared_ptr<ClientConfigList> getClientConfigListOption(config_option_t option) const override { return nullptr; }
    std::shared_ptr<DirectoryConfigList> getDirectoryTweakOption(config_option_t option) const override { return nullptr; }
    void updateConfigFromDatabase(std::shared_ptr<Database> database) override {};
    std::string getOrigValue(const std::string& item) const override { return ""; }
    void setOrigValue(const std::string& item, const std::string& value) override { }
    void setOrigValue(const std::string& item, std::string_view value) override { }
    void setOrigValue(const std::string& item, bool value) override { }
    void setOrigValue(const std::string& item, int value) override { }
    bool hasOrigValue(const std::string& item) const override { return false; }
    MOCK_METHOD(std::shared_ptr<TranscodingProfileList>, getTranscodingProfileListOption, (config_option_t option), (const override));
};

#endif // __CONFIG_MOCK_H__
