//
// Created by droc101 on 10/18/25.
//

#pragma once

#include <cstdint>
#include <libassets/type/paramDefs/ParamDefinition.h>

class IntParamDefinition final: public ParamDefinition
{
    public:
        IntParamDefinition() = default;

        int32_t minimumValue = INT32_MIN;
        int32_t maximumValue = INT32_MAX;
        int32_t defaultValue = 0;
};
