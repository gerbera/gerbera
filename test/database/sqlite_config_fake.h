/*GRB*

    Gerbera - https://gerbera.io/

    sqlite_config_fake.h - this file is part of Gerbera.

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

/// \file sqlite_config_fake.h

#ifndef GERBERA_SQLITE_CONFIG_FAKE_H
#define GERBERA_SQLITE_CONFIG_FAKE_H

class SqliteConfigFake : public Config {
public:
    fs::path getConfigFilename() const override { return {}; }
    std::string getOption(ConfigVal option) const override
    {
        if (option == ConfigVal::SERVER_STORAGE_SQLITE_DATABASE_FILE) {
            return "/tmp/gerbera.db";
        }
        if (option == ConfigVal::SERVER_STORAGE_SQLITE_INIT_SQL_FILE) {
            return "sqlite3.sql";
        }
        if (option == ConfigVal::SERVER_STORAGE_SQLITE_RESTORE) {
            return "true";
        }
        if (option == ConfigVal::SERVER_STORAGE_SQLITE_DROP_FILE) {
            return "sqlite3-drop.sql";
        }
        if (option == ConfigVal::SERVER_STORAGE_SQLITE_UPGRADE_FILE) {
            return "sqlite3-upgrade.xml";
        }
        if (option == ConfigVal::SERVER_STORAGE_DRIVER) {
            return "sqlite3";
        }
        return {};
    }
    void addOption(ConfigVal option, const std::shared_ptr<ConfigOption>& optionValue) override { }
    std::int32_t getIntOption(ConfigVal option) const override { return 0; }
    std::uint32_t getUIntOption(ConfigVal option) const override { return 0; }
    long long getLongOption(ConfigVal option) const override { return 0; }
    unsigned long long getULongOption(ConfigVal option) const override { return 0; }
    std::shared_ptr<ConfigOption> getConfigOption(ConfigVal option) const override { return {}; }
    bool getBoolOption(ConfigVal option) const override
    {
        return option == ConfigVal::SERVER_STORAGE_SQLITE_RESTORE;
    }
    std::string generateUDN(const std::shared_ptr<Database>& database) override { return "uuid:12345678-1234-1234-1234-123456789abc"; };
    std::map<std::string, std::string> getDictionaryOption(ConfigVal option) const override { return {}; }
    std::vector<std::vector<std::pair<std::string, std::string>>> getVectorOption(ConfigVal option) const override { return {}; }
    std::vector<std::string> getArrayOption(ConfigVal option) const override { return {}; }
    std::vector<std::shared_ptr<AutoscanDirectory>> getAutoscanListOption(ConfigVal option) const override { return {}; }
    std::shared_ptr<BoxLayoutList> getBoxLayoutListOption(ConfigVal option) const override { return nullptr; }
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
    std::shared_ptr<TranscodingProfileList> getTranscodingProfileListOption(ConfigVal option) const override { return nullptr; }
    std::shared_ptr<DynamicContentList> getDynamicContentListOption(ConfigVal option) const override { return nullptr; }
    void registerNode(const std::string& xmlPath) override {};
};

#endif //GERBERA_SQLITE_CONFIG_FAKE_H
