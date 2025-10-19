//
// Created by droc101 on 10/18/25.
//

#pragma once

#include <libassets/type/paramDefs/ParamDefinition.h>

class BoolParamDefinition final: public ParamDefinition
{
    public:
        BoolParamDefinition() = default;

        bool defaultValue = true;
};
