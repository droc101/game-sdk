//
// Created by droc101 on 10/19/25.
//

#pragma once

#include <libassets/type/OptionDefinition.h>
#include <libassets/type/paramDefs/ParamDefinition.h>
#include <string>
#include <utility>

class OptionParamDefinition final: public ParamDefinition
{
    public:
        OptionParamDefinition() = default;
        OptionParamDefinition(std::string &&optionListName, std::string &&defaultValue):
            optionListName(std::move(optionListName)),
            defaultValue(std::move(defaultValue))
        {}

        std::string optionListName;
        OptionDefinition *definition = nullptr;
        std::string defaultValue;
};
