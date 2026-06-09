//
// Created by droc101 on 6/9/26.
//

#pragma once

#include <libassets/type/renderDefs/values/BasicDefinitionValue.h>
#include <string>

class BoolDefinitionValue: public BasicDefinitionValue<bool>
{
    public:
        BoolDefinitionValue() = default;
        explicit BoolDefinitionValue(const bool &value): BasicDefinitionValue(value) {}
        BoolDefinitionValue(const nlohmann::json &json, const std::string &key, const bool &defaultValue);
};
