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
///\brief Definitions of the ConfigManager class.

#ifndef __CONFIG_OPTIONS_H__
#define __CONFIG_OPTIONS_H__

#include <map>
#include <string>
#include <vector>

#include "autoscan.h"
#include "exceptions.h"
#include <assert.h>

#include "transcoding/transcoding.h"

class ConfigOption {
public:
    virtual std::string getOption()
    {
        throw std::runtime_error("Wrong option type");
    }

    virtual int getIntOption()
    {
        throw std::runtime_error("Wrong option type");
    }

    virtual bool getBoolOption()
    {
        throw std::runtime_error("Wrong option type");
    }

    virtual std::map<std::string, std::string> getDictionaryOption()
    {
        throw std::runtime_error("Wrong option type");
    }

    virtual std::shared_ptr<AutoscanList> getAutoscanListOption()
    {
        throw std::runtime_error("Wrong option type");
    }

    virtual std::vector<std::string> getStringArrayOption()
    {
        throw std::runtime_error("Wrong option type");
    }

    virtual std::shared_ptr<TranscodingProfileList> getTranscodingProfileListOption()
    {
        throw std::runtime_error("Wrong option type");
    }
};

class Option : public ConfigOption {
public:
    Option(std::string option) { this->option = option; }

    std::string getOption() override { return option; }

protected:
    std::string option;
};

class IntOption : public ConfigOption {
public:
    IntOption(int option) { this->option = option; }

    int getIntOption() override { return option; }

protected:
    int option;
};

class BoolOption : public ConfigOption {
public:
    BoolOption(bool option) { this->option = option; }

    bool getBoolOption() override { return option; }

protected:
    BoolOption() = default;
    bool option;
};

class DictionaryOption : public ConfigOption {
public:
    DictionaryOption(std::map<std::string, std::string> option) { this->option = option; }

    std::map<std::string, std::string> getDictionaryOption() override { return option; }

protected:
    std::map<std::string, std::string> option;
};

class StringArrayOption : public ConfigOption {
public:
    StringArrayOption(std::vector<std::string> option)
    {
        this->option = option;
    }

    std::vector<std::string> getStringArrayOption() override
    {
        return option;
    }

protected:
    std::vector<std::string> option;
};

class AutoscanListOption : public ConfigOption {
public:
    AutoscanListOption(std::shared_ptr<AutoscanList> option)
    {
        this->option = option;
    }

    std::shared_ptr<AutoscanList> getAutoscanListOption() override { return option; }

protected:
    std::shared_ptr<AutoscanList> option;
};

class TranscodingProfileListOption : public ConfigOption {
public:
    TranscodingProfileListOption(std::shared_ptr<TranscodingProfileList> option)
    {
        this->option = option;
    }

    std::shared_ptr<TranscodingProfileList> getTranscodingProfileListOption() override
    {
        return option;
    }

protected:
    std::shared_ptr<TranscodingProfileList> option;
};

#endif // __CONFIG_MANAGER_H__
