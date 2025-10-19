//
// Created by droc101 on 10/18/25.
//

#pragma once

#include <cstdint>
#include <libassets/type/paramDefs/ParamDefinition.h>
#include <string>

class StringParamDefinition final: public ParamDefinition
{
    public:
        enum class StringParamHint : uint8_t
        {
            NONE,
            TEXTURE,
            MODEL,
            SOUND,
            ACTOR
        };

        StringParamDefinition() = default;

        StringParamHint hintType = StringParamHint::NONE;
        std::string defaultValue;
};
