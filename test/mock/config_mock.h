#ifndef __CONFIG_MOCK_H__
#define __CONFIG_MOCK_H__

#include <gtest/gtest.h>

#include "config/config.h"

using namespace ::testing;

class ConfigMock : public Config
{
public:
    fs::path getConfigFilename() const override { return ""; }
    std::string getOption(config_option_t option) { return ""; }
    int getIntOption(config_option_t option) { return 0; }
    bool getBoolOption(config_option_t option) { return false; }
    std::map<std::string, std::string> getDictionaryOption(config_option_t option) { return std::map<std::string, std::string>(); }
    std::vector<std::string> getArrayOption(config_option_t option) { return std::vector<std::string>(); }
    std::shared_ptr<AutoscanList> getAutoscanListOption(config_option_t option) { return nullptr; }
    std::shared_ptr<TranscodingProfileList> getTranscodingProfileListOption(config_option_t option) { return std::make_shared<TranscodingProfileList>(); }
};

#endif // __CONFIG_MOCK_H__
