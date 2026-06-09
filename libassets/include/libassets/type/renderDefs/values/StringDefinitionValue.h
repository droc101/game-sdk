//
// Created by droc101 on 6/9/26.
//

#pragma once

#include <libassets/type/renderDefs/values/BasicDefinitionValue.h>
#include <string>

class StringDefinitionValue: public BasicDefinitionValue<std::string>
{
    public:
        StringDefinitionValue() = default;
        StringDefinitionValue(const nlohmann::json &json, const std::string &key, const std::string &defaultValue);
};
