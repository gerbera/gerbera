/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    config_options.h - this file is part of MediaTomb.
    
    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>
    
    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>
    
    MediaTomb is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.
    
    MediaTomb is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    version 2 along with MediaTomb; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
    
    $Id$
*/

/// \file config_options.h
///\brief Definitions of the ConfigOption class.

#ifndef __CONFIG_OPTIONS_H__
#define __CONFIG_OPTIONS_H__

#include <map>
#include <string>
#include <utility>
#include <vector>

// forward declaration
class AutoscanList;
class ClientConfigList;
class TranscodingProfileList;

class ConfigOption {
public:
    virtual std::string getOption() const
    {
        throw std::runtime_error("Wrong option type string");
    }

    virtual int getIntOption() const
    {
        throw std::runtime_error("Wrong option type int");
    }

    virtual bool getBoolOption() const
    {
        throw std::runtime_error("Wrong option type bool");
    }

    virtual std::map<std::string, std::string> getDictionaryOption() const
    {
        throw std::runtime_error("Wrong option type dictionary");
    }

    virtual std::shared_ptr<AutoscanList> getAutoscanListOption() const
    {
        throw std::runtime_error("Wrong option type autoscan list");
    }

    virtual std::shared_ptr<ClientConfigList> getClientConfigListOption() const
    {
        throw std::runtime_error("Wrong option type client list");
    }

    virtual std::vector<std::string> getArrayOption() const
    {
        throw std::runtime_error("Wrong option type array");
    }

    virtual std::shared_ptr<TranscodingProfileList> getTranscodingProfileListOption() const
    {
        throw std::runtime_error("Wrong option type transcoding list");
    }

    virtual ~ConfigOption() = default;
};

class Option : public ConfigOption {
public:
    explicit Option(const std::string& option) { this->option = option; }

    std::string getOption() const override { return option; }

protected:
    std::string option;
};

class IntOption : public ConfigOption {
public:
    explicit IntOption(int option) { this->option = option; }

    int getIntOption() const override { return option; }

protected:
    int option;
};

class BoolOption : public ConfigOption {
public:
    explicit BoolOption(bool option) { this->option = option; }

    bool getBoolOption() const override { return option; }

protected:
    BoolOption() = default;
    bool option;
};

class DictionaryOption : public ConfigOption {
public:
    explicit DictionaryOption(const std::map<std::string, std::string>& option) { this->option = option; }

    std::map<std::string, std::string> getDictionaryOption() const override { return option; }

protected:
    std::map<std::string, std::string> option;
};

class ArrayOption : public ConfigOption {
public:
    explicit ArrayOption(const std::vector<std::string>& option)
    {
        this->option = option;
    }

    std::vector<std::string> getArrayOption() const override
    {
        return option;
    }

protected:
    std::vector<std::string> option;
};

class AutoscanListOption : public ConfigOption {
public:
    explicit AutoscanListOption(std::shared_ptr<AutoscanList> option)
    {
        this->option = std::move(option);
    }

    std::shared_ptr<AutoscanList> getAutoscanListOption() const override { return option; }

protected:
    std::shared_ptr<AutoscanList> option;
};

class ClientConfigListOption : public ConfigOption {
public:
    explicit ClientConfigListOption(std::shared_ptr<ClientConfigList> option)
    {
        this->option = std::move(option);
    }

    std::shared_ptr<ClientConfigList> getClientConfigListOption() const override { return option; }

protected:
    std::shared_ptr<ClientConfigList> option;
};

class TranscodingProfileListOption : public ConfigOption {
public:
    explicit TranscodingProfileListOption(std::shared_ptr<TranscodingProfileList> option)
    {
        this->option = std::move(option);
    }

    std::shared_ptr<TranscodingProfileList> getTranscodingProfileListOption() const override
    {
        return option;
    }

protected:
    std::shared_ptr<TranscodingProfileList> option;
};

#endif // __CONFIG_MANAGER_H__
