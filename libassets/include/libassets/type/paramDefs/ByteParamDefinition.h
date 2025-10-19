//
// Created by droc101 on 10/18/25.
//

#pragma once

#include <cstdint>
#include <libassets/type/paramDefs/ParamDefinition.h>

class ByteParamDefinition final: public ParamDefinition
{
    public:
        ByteParamDefinition() = default;

        uint8_t minimumValue = 0;
        uint8_t maximumValue = UINT8_MAX;
        uint8_t defaultValue = 0;
};
