#ifndef __CONFIG_MOCK_H__
#define __CONFIG_MOCK_H__

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "config/config.h"

using namespace ::testing;

class ConfigMock final : public Config {
public:
    fs::path getConfigFilename() const override { return ""; }
    MOCK_METHOD(std::string, getOption, (config_option_t option), (override));
    int getIntOption(config_option_t option) override { return 0; }
    bool getBoolOption(config_option_t option) override { return false; }
    std::map<std::string, std::string> getDictionaryOption(config_option_t option) override { return std::map<std::string, std::string>(); }
    std::vector<std::string> getArrayOption(config_option_t option) override { return std::vector<std::string>(); }
    std::shared_ptr<AutoscanList> getAutoscanListOption(config_option_t option) override { return nullptr; }
    std::shared_ptr<ClientConfigList> getClientConfigListOption(config_option_t option) override { return nullptr; }
    MOCK_METHOD(std::shared_ptr<TranscodingProfileList>, getTranscodingProfileListOption, (config_option_t option), (override));
};

#endif // __CONFIG_MOCK_H__
