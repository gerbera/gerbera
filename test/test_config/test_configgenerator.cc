#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include <config_manager.h>
#include <config/config_generator.h>
#include <fstream>

using namespace ::testing;

class ConfigGeneratorTest : public ::testing::Test {

public:
    ConfigGeneratorTest() {};
    virtual ~ConfigGeneratorTest() {};

    virtual void SetUp() {
       subject = new ConfigGenerator();
    }

    virtual void TearDown() {

    };

    std::string mockConfigXml() {
        std::ifstream t("fixtures/mock-config.xml");
        std::string str((std::istreambuf_iterator<char>(t)),
                        std::istreambuf_iterator<char>());

        return str;
    }

    ConfigGenerator* subject;
};


TEST_F(ConfigGeneratorTest, GeneratesStringForConfiguration) {
    std::string mockConfig = mockConfigXml();
    std::string defaultConfig = subject->generate();

    ASSERT_NE(nullptr, defaultConfig.c_str());
    EXPECT_STREQ(mockConfig.c_str(), defaultConfig.c_str());
}
