/*MT*

    MediaTomb - http://www.mediatomb.cc/

    config_options.h - this file is part of MediaTomb.

    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>

    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>

    Copyright (C) 2016-2023 Gerbera Contributors

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
#include <vector>

#include "transcoding/transcoding.h"

// forward declaration
class AutoscanDirectory;
class AutoscanList;
class ClientConfigList;
class DirectoryConfigList;
class DynamicContentList;

class ConfigOption {
public:
    virtual ~ConfigOption() = default;

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

    virtual std::vector<std::vector<std::pair<std::string, std::string>>> getVectorOption(bool forEdit = false) const
    {
        throw std::runtime_error("Wrong option type array");
    }

    virtual std::vector<AutoscanDirectory> getAutoscanListOption() const
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

    virtual std::shared_ptr<DynamicContentList> getDynamicContentListOption() const
    {
        throw std::runtime_error("Wrong option type dynamic content list");
    }
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
    explicit DictionaryOption(const std::map<std::string, std::string>& option)
        : option(option)
        , origSize(option.size())
    {
        std::size_t i = 0;
        for (auto&& [key, val] : option) {
            this->indexMap[i] = key;
            i++;
        }
    }

    std::map<std::string, std::string> getDictionaryOption(bool forEdit = false) const override;

    std::string getKey(std::size_t index)
    {
        return indexMap[index];
    }

    std::size_t getEditSize() const;

    void setKey(std::size_t keyIndex, const std::string& newKey);

    void setValue(std::size_t keyIndex, std::string value);

private:
    std::map<std::string, std::string> option;
    std::size_t origSize;
    std::map<std::size_t, std::string> indexMap;
};

class VectorOption : public ConfigOption {
public:
    explicit VectorOption(const std::vector<std::vector<std::pair<std::string, std::string>>>& option)
        : option(option)
        , origSize(option.size())
    {
        for (std::size_t i = 0; i < origSize; i++) {
            this->indexMap[i] = i;
        }
    }

    std::size_t getIndex(std::size_t index)
    {
        return index < indexMap.size() ? indexMap[index] : indexMap.size();
    }

    void setValue(std::size_t optionIndex, std::size_t entryIndex, std::string value);

    std::vector<std::vector<std::pair<std::string, std::string>>> getVectorOption(bool forEdit = false) const override;

    std::size_t getEditSize() const;

private:
    std::vector<std::vector<std::pair<std::string, std::string>>> option;
    std::size_t origSize;
    std::map<std::size_t, std::size_t> indexMap;
};

class ArrayOption : public ConfigOption {
public:
    explicit ArrayOption(const std::vector<std::string>& option)
        : option(option)
        , origSize(option.size())
    {
        for (std::size_t i = 0; i < origSize; i++) {
            this->indexMap[i] = i;
        }
    }

    std::vector<std::string> getArrayOption(bool forEdit = false) const override;

    std::size_t getIndex(std::size_t index)
    {
        return index < indexMap.size() ? indexMap[index] : indexMap.size();
    }

    std::size_t getEditSize() const;

    void setItem(std::size_t index, const std::string& value);

private:
    std::vector<std::string> option;
    std::size_t origSize {};
    std::map<std::size_t, std::size_t> indexMap;
};

class AutoscanListOption : public ConfigOption {
public:
    explicit AutoscanListOption(std::vector<AutoscanDirectory> autoscans_)
        : autoscans(std::move(autoscans_))
    {
    }

    [[nodiscard]] std::vector<AutoscanDirectory> getAutoscanListOption() const override { return autoscans; }

private:
    std::vector<AutoscanDirectory> autoscans;
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

protected:
    std::shared_ptr<TranscodingProfileList> option;
};

class DynamicContentListOption : public ConfigOption {
public:
    explicit DynamicContentListOption(std::shared_ptr<DynamicContentList> option)
        : option(std::move(option))
    {
    }

    std::shared_ptr<DynamicContentList> getDynamicContentListOption() const override
    {
        return option;
    }

protected:
    std::shared_ptr<DynamicContentList> option;
};

#endif // __CONFIG_OPTIONS_H__
