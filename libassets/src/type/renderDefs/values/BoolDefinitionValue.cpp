//
// Created by droc101 on 6/9/26.
//

#include <libassets/type/renderDefs/values/BoolDefinitionValue.h>
#include <string>

BoolDefinitionValue::BoolDefinitionValue(const nlohmann::json &json, const std::string &key, const bool &defaultValue)
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
