//
// Created by droc101 on 6/9/26.
//

#include <libassets/type/renderDefs/values/StringDefinitionValue.h>
#include <string>

StringDefinitionValue::StringDefinitionValue(const nlohmann::json &json,
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
