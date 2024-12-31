/*GRB*

    Gerbera - https://gerbera.io/

    mysql_config_fake.h - this file is part of Gerbera.

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

/// \file my_sql_config_fake.h

#ifndef GERBERA_MYSQL_CONFIG_FAKE_H
#define GERBERA_MYSQL_CONFIG_FAKE_H

class MySQLConfigFake : public Config {
public:
    fs::path getConfigFilename() const override { return {}; }
    std::string getOption(ConfigVal option) const override
    {
        if (option == ConfigVal::SERVER_STORAGE_MYSQL_HOST) {
            return "hydra.home";
        }
        if (option == ConfigVal::SERVER_STORAGE_MYSQL_USERNAME) {
            return "root";
        }
        if (option == ConfigVal::SERVER_STORAGE_DRIVER) {
            return "mysql";
        }
        if (option == ConfigVal::SERVER_STORAGE_MYSQL_INIT_SQL_FILE) {
            return "mysql.sql";
        }
        if (option == ConfigVal::SERVER_STORAGE_MYSQL_UPGRADE_FILE) {
            return "mysql-upgrade.xml";
        }
        //        if (option == ConfigVal::SERVER_STORAGE_MYSQL_DATABASE) {
        //            return "gerbera";
        //        }
        return {};
    }
    void addOption(ConfigVal option, const std::shared_ptr<ConfigOption>& optionValue) override { }
    std::int32_t getIntOption(ConfigVal option) const override { return 0; }
    std::uint32_t getUIntOption(ConfigVal option) const override { return 0; }
    std::int64_t getLongOption(ConfigVal option) const override { return 0; }
    std::shared_ptr<ConfigOption> getConfigOption(ConfigVal option) const override { return {}; }
    bool getBoolOption(ConfigVal option) const override { return false; }
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
    void setOrigValue(const std::string& item, std::int64_t value) override { }
    bool hasOrigValue(const std::string& item) const override { return false; }
    std::shared_ptr<TranscodingProfileList> getTranscodingProfileListOption(ConfigVal option) const override { return nullptr; }
    std::shared_ptr<DynamicContentList> getDynamicContentListOption(ConfigVal option) const override { return nullptr; }
};

#endif //GERBERA_MYSQL_CONFIG_FAKE_H
