/*MT*

    MediaTomb - http://www.mediatomb.cc/

    config_options.h - this file is part of MediaTomb.

    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>

    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>

    Copyright (C) 2016-2026 Gerbera Contributors

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

/// @file config/config_options.h
/// @brief Definitions of the ConfigOption class.

#ifndef __CONFIG_OPTIONS_H__
#define __CONFIG_OPTIONS_H__

#include <fmt/format.h>
#include <map>
#include <memory>
#include <vector>

#include "config_int_types.h"

// forward declarations
class AutoscanDirectory;
class BoxLayoutList;
class ClientConfigList;
class DirectoryConfigList;
class DynamicContentList;
class TranscodingProfileList;

/// @brief Base class for option values
class ConfigOption {
public:
    virtual ~ConfigOption() = default;

    virtual std::string getOption() const
    {
        throw std::runtime_error("Wrong option type string");
    }

    virtual IntOptionType getIntOption() const
    {
        throw std::runtime_error("Wrong option type int");
    }
    virtual UIntOptionType getUIntOption() const
    {
        throw std::runtime_error("Wrong option type uint");
    }
    virtual LongOptionType getLongOption() const
    {
        throw std::runtime_error("Wrong option type long");
    }
    virtual ULongOptionType getULongOption() const
    {
        throw std::runtime_error("Wrong option type ulong");
    }

    virtual bool getBoolOption() const
    {
        throw std::runtime_error("Wrong option type bool");
    }

    virtual std::map<std::string, std::string> getDictionaryOption(bool forEdit = false) const
    {
        throw std::runtime_error("Wrong option type Dictionary");
    }

    virtual std::vector<std::vector<std::pair<std::string, std::string>>> getVectorOption(bool forEdit = false) const
    {
        throw std::runtime_error("Wrong option type Vector");
    }

    virtual std::vector<std::shared_ptr<AutoscanDirectory>> getAutoscanListOption() const
    {
        throw std::runtime_error("Wrong option type Autoscan list");
    }

    virtual std::shared_ptr<ClientConfigList> getClientConfigListOption() const
    {
        throw std::runtime_error("Wrong option type Client list");
    }

    virtual std::shared_ptr<BoxLayoutList> getBoxLayoutListOption() const
    {
        throw std::runtime_error("Wrong option type BoxLayout list");
    }

    virtual std::vector<std::string> getArrayOption(bool forEdit = false) const
    {
        throw std::runtime_error("Wrong option type Array");
    }

    virtual std::shared_ptr<TranscodingProfileList> getTranscodingProfileListOption() const
    {
        throw std::runtime_error("Wrong option type Transcoding list");
    }

    virtual std::shared_ptr<DirectoryConfigList> getDirectoryTweakOption() const
    {
        throw std::runtime_error("Wrong option type Directory list");
    }

    virtual std::shared_ptr<DynamicContentList> getDynamicContentListOption() const
    {
        throw std::runtime_error("Wrong option type DynamicContent list");
    }
};

/// @brief Implementation of option value for strings
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

/// @brief Implementation of option value for signed integers
class IntOption : public ConfigOption {
public:
    explicit IntOption(IntOptionType option, std::string optionString = "")
        : option(option)
        , optionString(std::move(optionString))
    {
    }

    std::string getOption() const override { return optionString.empty() ? fmt::to_string(option) : optionString; }

    IntOptionType getIntOption() const override { return option; }

private:
    IntOptionType option;
    std::string optionString;
};

/// @brief Implementation of option value for unsigned integers
class UIntOption : public ConfigOption {
public:
    explicit UIntOption(UIntOptionType option, std::string optionString = "")
        : option(option)
        , optionString(std::move(optionString))
    {
    }

    std::string getOption() const override { return optionString.empty() ? fmt::to_string(option) : optionString; }

    UIntOptionType getUIntOption() const override { return option; }

private:
    UIntOptionType option;
    std::string optionString;
};

/// @brief Implementation of option value for long integers
class LongOption : public ConfigOption {
public:
    explicit LongOption(LongOptionType option, std::string optionString = "")
        : option(option)
        , optionString(std::move(optionString))
    {
    }

    std::string getOption() const override { return optionString.empty() ? fmt::to_string(option) : optionString; }

    LongOptionType getLongOption() const override { return option; }

private:
    LongOptionType option;
    std::string optionString;
};

/// @brief Implementation of option value for unsigned long long integers
class ULongOption : public ConfigOption {
public:
    explicit ULongOption(ULongOptionType option, std::string optionString = "")
        : option(option)
        , optionString(std::move(optionString))
    {
    }

    std::string getOption() const override { return optionString.empty() ? fmt::to_string(option) : optionString; }

    ULongOptionType getULongOption() const override { return option; }

private:
    ULongOptionType option;
    std::string optionString;
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

/// @brief Implementation of option value for dictionaries
class DictionaryOption : public ConfigOption {
public:
    explicit DictionaryOption() = default;
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

/// @brief Implementation of option value for vectors
class VectorOption : public ConfigOption {
public:
    explicit VectorOption() = default;
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

    /// @brief Change key of a vector entry
    void setKey(std::size_t optionIndex, std::size_t entryIndex, std::string key);
    /// @brief Change value of a vector entry
    void setValue(std::size_t optionIndex, std::size_t entryIndex, std::string value);

    std::vector<std::vector<std::pair<std::string, std::string>>> getVectorOption(bool forEdit = false) const override;

    std::size_t getEditSize() const;

private:
    std::vector<std::vector<std::pair<std::string, std::string>>> option;
    std::size_t origSize;
    std::map<std::size_t, std::size_t> indexMap;
};

/// @brief Implementation of option value for arrays
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

/// @brief Implementation of option value for autoscan lists
class AutoscanListOption : public ConfigOption {
public:
    explicit AutoscanListOption(std::vector<std::shared_ptr<AutoscanDirectory>> autoscans_)
        : autoscans(std::move(autoscans_))
    {
    }

    [[nodiscard]] std::vector<std::shared_ptr<AutoscanDirectory>> getAutoscanListOption() const override { return autoscans; }

private:
    std::vector<std::shared_ptr<AutoscanDirectory>> autoscans;
};

/// @brief Implementation of option value for boxlayout lists
class BoxLayoutListOption : public ConfigOption {
public:
    explicit BoxLayoutListOption(std::shared_ptr<BoxLayoutList> option)
        : option(std::move(option))
    {
    }
    std::shared_ptr<BoxLayoutList> getBoxLayoutListOption() const override { return option; }

private:
    std::shared_ptr<BoxLayoutList> option;
};

/// @brief Implementation of option value for client lists
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

/// @brief Implementation of option value for directory tweaks
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

/// @brief Implementation of option value for transcoding profile lists
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

/// @brief Implementation of option value for dynamic tree content lists
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
