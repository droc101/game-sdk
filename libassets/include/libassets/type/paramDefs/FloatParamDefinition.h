//
// Created by droc101 on 10/18/25.
//

#pragma once

#include <cfloat>
#include <libassets/type/paramDefs/ParamDefinition.h>

class FloatParamDefinition final: public ParamDefinition
{
    public:
        FloatParamDefinition() = default;

        float minimumValue = FLT_MAX;
        float maximumValue = -FLT_MAX;
        float defaultValue = 0.0f;
        float step = 0.1f;
};
