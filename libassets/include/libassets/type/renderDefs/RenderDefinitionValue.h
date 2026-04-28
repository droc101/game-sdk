//
// Created by droc101 on 4/28/26.
//

#pragma once

#include <concepts>
#include <libassets/type/Color.h>
#include <libassets/type/Param.h>
#include <string>

template<typename T> concept RDVTypeTemplate = std::same_as<T, float> ||
                                               std::same_as<T, bool> ||
                                               std::same_as<T, std::string> ||
                                               std::same_as<T, Color>;

template<RDVTypeTemplate T> class RenderDefinitionValue
{
    public:
        RenderDefinitionValue() = default;
        RenderDefinitionValue(const nlohmann::json &json, const std::string &key, const T &defaultValue)
        {
            if constexpr (std::same_as<T, std::string>)
            {
                if (json.contains(key) && json.at(key).type() == nlohmann::detail::value_t::string)
                {
                    const std::string jsonValue = json.value(key, defaultValue);
                    if (value.starts_with("$"))
                    {
                        usesParam = true;
                        paramName = jsonValue.substr(1, jsonValue.length() - 1);
                    } else
                    {
                        value = jsonValue;
                    }
                } else
                {
                    value = defaultValue;
                }
            } else if constexpr (std::same_as<T, Color>)
            {
                if (json.contains(key))
                {
                    if (json.at(key).type() == nlohmann::detail::value_t::number_unsigned ||
                        json.at(key).type() == nlohmann::detail::value_t::number_integer)
                    {
                        value = Color(json.value(key, -1u));
                    } else if (json.at(key).type() == nlohmann::detail::value_t::string)
                    {
                        const std::string colorValue = json.value(key, "");
                        if (colorValue.starts_with("$"))
                        {
                            usesParam = true;
                            paramName = colorValue.substr(1, colorValue.length() - 1);
                        } else
                        {
                            value = defaultValue;
                        }
                    }
                } else
                {
                    value = defaultValue;
                }
            } else if constexpr (std::same_as<T, bool>)
            {
                if (json.contains(key))
                {
                    if (json.at(key).type() == nlohmann::detail::value_t::boolean)
                    {
                        value = json.value(key, defaultValue);
                    } else if (json.at(key).type() == nlohmann::detail::value_t::string)
                    {
                        const std::string boolValue = json.value(key, "");
                        if (boolValue.starts_with("$"))
                        {
                            usesParam = true;
                            paramName = boolValue.substr(1, boolValue.length() - 1);
                        } else
                        {
                            value = defaultValue;
                        }
                    }
                } else
                {
                    value = defaultValue;
                }
            } else if constexpr (std::same_as<T, float>)
            {
                if (json.contains(key))
                {
                    if (json.at(key).type() == nlohmann::detail::value_t::number_float ||
                        json.at(key).type() == nlohmann::detail::value_t::number_unsigned ||
                        json.at(key).type() == nlohmann::detail::value_t::number_integer)
                    {
                        value = json.value(key, defaultValue);
                    } else if (json.at(key).type() == nlohmann::detail::value_t::string)
                    {
                        const std::string floatValue = json.value(key, "");
                        if (floatValue.starts_with("$"))
                        {
                            usesParam = true;
                            paramName = floatValue.substr(1, floatValue.length() - 1);
                        } else
                        {
                            value = defaultValue;
                        }
                    }
                } else
                {
                    value = defaultValue;
                }
            }
        }

        T Get(const KvList &params, const T &defaultValue) const
        {
            if (usesParam)
            {
                if (params.contains(paramName))
                {
                    return params.at(paramName).Get<T>(defaultValue);
                }
            } else
            {
                return value;
            }
            return defaultValue;
        }

    private:
        bool usesParam = false;
        std::string paramName{};
        T value{};
};
