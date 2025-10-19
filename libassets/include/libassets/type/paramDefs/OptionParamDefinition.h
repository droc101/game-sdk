//
// Created by droc101 on 10/19/25.
//

#pragma once

#include <cstddef>
#include <libassets/type/OptionDefinition.h>
#include <libassets/type/paramDefs/ParamDefinition.h>
#include <string>

class OptionParamDefinition final: public ParamDefinition
{
    public:
        OptionParamDefinition() = default;

        std::string optionListName;
        OptionDefinition *definition = nullptr;
        std::string defaultValue;
};
