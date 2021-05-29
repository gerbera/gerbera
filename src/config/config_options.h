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
#include <vector>

#include "transcoding/transcoding.h"

// forward declaration
class AutoscanList;
class ClientConfigList;
class DirectoryConfigList;

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

    virtual std::map<std::string, std::string> getDictionaryOption(bool forEdit = false) const
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

    virtual std::vector<std::string> getArrayOption(bool forEdit = false) const
    {
        throw std::runtime_error("Wrong option type array");
    }

    virtual std::shared_ptr<TranscodingProfileList> getTranscodingProfileListOption() const
    {
        throw std::runtime_error("Wrong option type transcoding list");
    }

    virtual std::shared_ptr<DirectoryConfigList> getDirectoryTweakOption() const
    {
        throw std::runtime_error("Wrong option type directory list");
    }

    virtual ~ConfigOption() = default;
};

class Option : public ConfigOption {
public:
    explicit Option(std::string option)
        : option(std::move(option))
    {
    }

    std::string getOption() const override { return option; }

private:
    std::string option;
};

class IntOption : public ConfigOption {
public:
    explicit IntOption(int option)
        : option(option)
    {
    }

    int getIntOption() const override { return option; }

private:
    int option;
};

class BoolOption : public ConfigOption {
public:
    explicit BoolOption(bool option)
        : option(option)
    {
    }

    bool getBoolOption() const override { return option; }

private:
    bool option;
};

class DictionaryOption : public ConfigOption {
public:
    explicit DictionaryOption(std::map<std::string, std::string> option)
        : option(std::move(option))
    {
        this->origSize = this->option.size();
        size_t i = 0;
        for (auto&& [key, val] : this->option) {
            this->indexMap[i] = key;
            i++;
        }
    }

    std::map<std::string, std::string> getDictionaryOption(bool forEdit = false) const override;

    std::string getKey(size_t index)
    {
        return indexMap[index];
    }

    size_t getEditSize() const;

    void setKey(size_t keyIndex, const std::string& newKey);

    void setValue(size_t keyIndex, const std::string& value);

private:
    std::map<std::string, std::string> option;
    size_t origSize;
    std::map<size_t, std::string> indexMap;
};

class ArrayOption : public ConfigOption {
public:
    explicit ArrayOption(std::vector<std::string> option)
        : option(std::move(option))
        , origSize()
    {
        this->origSize = this->option.size();
        for (size_t i = 0; i < this->origSize; i++) {
            this->indexMap[i] = i;
        }
    }

    std::vector<std::string> getArrayOption(bool forEdit = false) const override;

    size_t getIndex(size_t index)
    {
        return index < indexMap.size() ? indexMap[index] : indexMap.size();
    }

    size_t getEditSize() const;

    void setItem(size_t index, const std::string& value);

private:
    std::vector<std::string> option;
    size_t origSize;
    std::map<size_t, size_t> indexMap;
};

class AutoscanListOption : public ConfigOption {
public:
    explicit AutoscanListOption(std::shared_ptr<AutoscanList> option)
        : option(std::move(option))
    {
    }

    std::shared_ptr<AutoscanList> getAutoscanListOption() const override { return option; }

private:
    std::shared_ptr<AutoscanList> option;
};

class ClientConfigListOption : public ConfigOption {
public:
    explicit ClientConfigListOption(std::shared_ptr<ClientConfigList> option)
        : option(std::move(option))
    {
    }
    std::shared_ptr<ClientConfigList> getClientConfigListOption() const override { return option; }

private:
    std::shared_ptr<ClientConfigList> option;
};

class DirectoryTweakOption : public ConfigOption {
public:
    explicit DirectoryTweakOption(std::shared_ptr<DirectoryConfigList> option)
        : option(std::move(option))
    {
    }
    std::shared_ptr<DirectoryConfigList> getDirectoryTweakOption() const override { return option; }

protected:
    std::shared_ptr<DirectoryConfigList> option;
};

class TranscodingProfileListOption : public ConfigOption {
public:
    explicit TranscodingProfileListOption(std::shared_ptr<TranscodingProfileList> option)
        : option(std::move(option))
    {
    }
    std::shared_ptr<TranscodingProfileList> getTranscodingProfileListOption() const override
    {
        return option;
    }

    void setKey(const std::string& oldKey, const std::string& newKey)
    {
        option->setKey(oldKey, newKey);
    }

protected:
    std::shared_ptr<TranscodingProfileList> option;
};

#endif // __CONFIG_MANAGER_H__
