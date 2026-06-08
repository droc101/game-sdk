//
// Created by droc101 on 5/25/26.
//

#include <cstddef>
#include <cstdint>
#include <libassets/type/Color.h>
#include <libassets/type/RenderDefinitionValue.h>
#include <string>

template<> void RenderDefinitionValue<std::string>::ConstructString(const nlohmann::json &json,
                                                                    const std::string &key,
                                                                    const std::string &defaultValue)
{
    if (json.contains(key) && json.at(key).type() == nlohmann::detail::value_t::string)
    {
        const std::string jsonValue = json.value(key, defaultValue);
        if (jsonValue.starts_with("$"))
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
}

template<> void RenderDefinitionValue<Color>::ConstructColor(const nlohmann::json &json,
                                                             const std::string &key,
                                                             const Color &defaultValue)
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
}

template<> void RenderDefinitionValue<bool>::ConstructBool(const nlohmann::json &json,
                                                           const std::string &key,
                                                           const bool &defaultValue)
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
}

template<> void RenderDefinitionValue<float>::ConstructFloat(const nlohmann::json &json,
                                                             const std::string &key,
                                                             const float &defaultValue)
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
            const std::string expr = json.value(key, "");
            ParseExpression(expr, defaultValue);
        }
    } else
    {
        value = defaultValue;
    }
}

template<> void RenderDefinitionValue<uint32_t>::ConstructUint32(const nlohmann::json &json,
                                                                 const std::string &key,
                                                                 const uint32_t &defaultValue)
{
    if (json.contains(key))
    {
        if (json.at(key).type() == nlohmann::detail::value_t::number_unsigned)
        {
            value = json.value(key, defaultValue);
        } else if (json.at(key).type() == nlohmann::detail::value_t::string)
        {
            const std::string expr = json.value(key, "");
            ParseExpression(expr, defaultValue);
        }
    } else
    {
        value = defaultValue;
    }
}
