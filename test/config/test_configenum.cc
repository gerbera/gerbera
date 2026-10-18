/*GRB*
    Gerbera - https://gerbera.io/

    Copyright (C) 2026 Gerbera Contributors

    Gerbera is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.
*/

#include "config/config_definition.h"
#include "config/config_val.h"
#include "config/setup/config_setup_enum.h"
#include "metadata/metadata_enums.h"

#include <gtest/gtest.h>
#include <pugixml.hpp>

class ConfigEnumSetupTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        definition = std::make_shared<ConfigDefinition>();
        definition->init(definition);
    }

    std::shared_ptr<ConfigDefinition> definition;
};

TEST_F(ConfigEnumSetupTest, ConvertsNamesAndNumericAliasesToTypedOptions)
{
    auto setup = definition->findConfigSetup<ConfigEnumSetup<UrlAppendMode>>(ConfigVal::SERVER_APPEND_PRESENTATION_URL_TO);
    for (const auto& [name, value] : std::map<std::string, UrlAppendMode> {
             { "none", UrlAppendMode::none }, { "ip", UrlAppendMode::ip }, { "port", UrlAppendMode::port }, { "1", UrlAppendMode::ip } }) {
        SCOPED_TRACE(name);
        auto option = std::dynamic_pointer_cast<EnumOption<UrlAppendMode>>(setup->newOption(name));
        ASSERT_NE(option, nullptr);
        EXPECT_EQ(option->getEnumOption(), value);
        EXPECT_EQ(option->getOption(), setup->mapEnumValue(value));
    }
}

TEST_F(ConfigEnumSetupTest, ReadsXmlAttributesAndConfiguredDefaults)
{
    auto setup = definition->findConfigSetup<ConfigEnumSetup<UrlAppendMode>>(ConfigVal::SERVER_APPEND_PRESENTATION_URL_TO);
    pugi::xml_document document;
    ASSERT_TRUE(document.load_string("<config><server><presentationURL append-to=' port '/></server></config>"));
    EXPECT_EQ(setup->getXmlContent(document, nullptr), UrlAppendMode::port);
    ASSERT_TRUE(document.load_string("<config/>"));
    EXPECT_EQ(setup->getXmlContent(document, nullptr), UrlAppendMode::none);
    setup->setDefaultValue(UrlAppendMode::ip);
    EXPECT_EQ(setup->getXmlContent(document, nullptr), UrlAppendMode::ip);
}

TEST_F(ConfigEnumSetupTest, RejectsUnsupportedNames)
{
    auto setup = definition->findConfigSetup<ConfigEnumSetup<UrlAppendMode>>(ConfigVal::SERVER_APPEND_PRESENTATION_URL_TO);
    for (const auto* value : { "", "unknown", "IP", "99" }) {
        SCOPED_TRACE(value);
        EXPECT_THROW(setup->newOption(value), std::runtime_error);
    }
}

#ifdef HAVE_TAGLIB
TEST_F(ConfigEnumSetupTest, MapsMp4BpmConflictSources)
{
    auto setup = definition->findConfigSetup<ConfigEnumSetup<BpmSource>>(ConfigVal::IMPORT_LIBOPTS_ID3_PREFER_AMBIGOUS_BPM_VALUE);
    ASSERT_NE(setup, nullptr);
    for (const auto& [name, value] : std::map<std::string, BpmSource> { { "tmpo", BpmSource::Tmpo }, { "bpm_tag", BpmSource::BpmTag } }) {
        auto option = std::dynamic_pointer_cast<EnumOption<BpmSource>>(setup->newOption(name));
        ASSERT_NE(option, nullptr);
        EXPECT_EQ(option->getEnumOption(), value);
        EXPECT_EQ(option->getOption(), name);
    }
    EXPECT_EQ(setup->getDefaultValue(), "bpm_tag");
    EXPECT_THROW(setup->newOption("unknown"), std::runtime_error);
    EXPECT_THROW(setup->newOption("BPM_TAG"), std::runtime_error);
}
#endif
