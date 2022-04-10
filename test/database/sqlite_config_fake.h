/*GRB*

Gerbera - https://gerbera.io/

    sqlite_config_fake.h - this file is part of Gerbera.

    Copyright (C) 2020-2022 Gerbera Contributors

    Gerbera is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.

    Gerbera is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Gerbera.  If not, see <http://www.gnu.org/licenses/>.
*/

/// \file sqlite_config_fake.h

#ifndef GERBERA_SQLITE_CONFIG_FAKE_H
#define GERBERA_SQLITE_CONFIG_FAKE_H

class SqliteConfigFake : public Config {
public:
    fs::path getConfigFilename() const override { return {}; }
    std::string getOption(config_option_t option) const override
    {
        if (option == CFG_SERVER_STORAGE_SQLITE_DATABASE_FILE) {
            return "/tmp/gerbera.db";
        }
        if (option == CFG_SERVER_STORAGE_SQLITE_INIT_SQL_FILE) {
            return "sqlite3.sql";
        }
        if (option == CFG_SERVER_STORAGE_SQLITE_RESTORE) {
            return "true";
        }
        if (option == CFG_SERVER_STORAGE_SQLITE_UPGRADE_FILE) {
            return "sqlite3-upgrade.xml";
        }
        if (option == CFG_SERVER_STORAGE_DRIVER) {
            return "sqlite3";
        }
        return {};
    }
    void addOption(config_option_t option, const std::shared_ptr<ConfigOption>& optionValue) override { }
    int getIntOption(config_option_t option) const override { return 0; }
    bool getBoolOption(config_option_t option) const override
    {
        if (option == CFG_SERVER_STORAGE_SQLITE_RESTORE) {
            return true;
        }
        return false;
    }
    std::map<std::string, std::string> getDictionaryOption(config_option_t option) const override { return {}; }
    std::vector<std::vector<std::pair<std::string, std::string>>> getVectorOption(config_option_t option) const override { return {}; }
    std::vector<std::string> getArrayOption(config_option_t option) const override { return {}; }
    std::vector<AutoscanDirectory> getAutoscanListOption(config_option_t option) const override { return {}; }
    std::shared_ptr<ClientConfigList> getClientConfigListOption(config_option_t option) const override { return nullptr; }
    std::shared_ptr<DirectoryConfigList> getDirectoryTweakOption(config_option_t option) const override { return nullptr; }
    void updateConfigFromDatabase(const std::shared_ptr<Database>& database) override { }
    std::string getOrigValue(const std::string& item) const override { return {}; }
    void setOrigValue(const std::string& item, const std::string& value) override { }
    void setOrigValue(const std::string& item, bool value) override { }
    void setOrigValue(const std::string& item, int value) override { }
    bool hasOrigValue(const std::string& item) const override { return false; }
    std::shared_ptr<TranscodingProfileList> getTranscodingProfileListOption(config_option_t option) const override { return nullptr; }
    std::shared_ptr<DynamicContentList> getDynamicContentListOption(config_option_t option) const override { return nullptr; }
};

#endif //GERBERA_SQLITE_CONFIG_FAKE_H
