/*GRB*

    Gerbera - https://gerbera.io/

    config_option_enum.h - this file is part of Gerbera.

    Copyright (C) 2023-2025 Gerbera Contributors

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

/// \file config_option_enum.h

#ifndef __CONFIG_OPTION_ENUM_H__
#define __CONFIG_OPTION_ENUM_H__

#include "config.h"
#include "config_options.h"

template <class Enum>
class EnumOption : public ConfigOption {
public:
    explicit EnumOption(Enum option, std::string optionString)
        : option(option)
        , optionString(std::move(optionString))
    {
    }

    std::string getOption() const override { return optionString.length() == 0 ? fmt::format("{}", option) : optionString; }

    Enum getEnumOption() const { return option; }

    static Enum getEnumOption(const std::shared_ptr<Config>& config, ConfigVal option)
    {
        auto optionValue = config->getConfigOption(option);
        auto optionEnumValue = std::dynamic_pointer_cast<EnumOption<Enum>>(optionValue);
        if (optionEnumValue) {
            return optionEnumValue->getEnumOption();
        }
        return Enum();
    }

private:
    Enum option;
    std::string optionString;
};

#endif // __CONFIG_OPTION_ENUM_H__
