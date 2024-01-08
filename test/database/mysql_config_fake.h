/*GRB*

Gerbera - https://gerbera.io/

    mysql_config_fake.h - this file is part of Gerbera.

    Copyright (C) 2020-2024 Gerbera Contributors

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

/// \file my_sql_config_fake.h

#ifndef GERBERA_MYSQL_CONFIG_FAKE_H
#define GERBERA_MYSQL_CONFIG_FAKE_H

class MySQLConfigFake : public Config {
public:
    fs::path getConfigFilename() const override { return {}; }
    std::string getOption(config_option_t option) const override
    {
        if (option == CFG_SERVER_STORAGE_MYSQL_HOST) {
            return "hydra.home";
        }
        if (option == CFG_SERVER_STORAGE_MYSQL_USERNAME) {
            return "root";
        }
        if (option == CFG_SERVER_STORAGE_DRIVER) {
            return "mysql";
        }
        if (option == CFG_SERVER_STORAGE_MYSQL_INIT_SQL_FILE) {
            return "mysql.sql";
        }
        if (option == CFG_SERVER_STORAGE_MYSQL_UPGRADE_FILE) {
            return "mysql-upgrade.xml";
        }
        //        if (option == CFG_SERVER_STORAGE_MYSQL_DATABASE) {
        //            return "gerbera";
        //        }
        return {};
    }
    void addOption(config_option_t option, const std::shared_ptr<ConfigOption>& optionValue) override { }
    int getIntOption(config_option_t option) const override { return 0; }
    std::shared_ptr<ConfigOption> getConfigOption(config_option_t option) const override { return {}; }
    bool getBoolOption(config_option_t option) const override { return false; }
    std::map<std::string, std::string> getDictionaryOption(config_option_t option) const override { return {}; }
    std::vector<std::vector<std::pair<std::string, std::string>>> getVectorOption(config_option_t option) const override { return {}; }
    std::vector<std::string> getArrayOption(config_option_t option) const override { return {}; }
    std::vector<std::shared_ptr<AutoscanDirectory>> getAutoscanListOption(config_option_t option) const override { return {}; }
    std::shared_ptr<BoxLayoutList> getBoxLayoutListOption(config_option_t option) const override { return nullptr; }
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

#endif //GERBERA_MYSQL_CONFIG_FAKE_H
