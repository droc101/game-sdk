//
// Created by droc101 on 10/18/25.
//

#pragma once

#include <libassets/type/Color.h>
#include <libassets/type/paramDefs/ParamDefinition.h>
#include <utility>

class ColorParamDefinition final: public ParamDefinition
{
    public:
        ColorParamDefinition() = default;
        ColorParamDefinition(Color &&defaultValue, const bool showAlpha):
            defaultValue(std::move(defaultValue)),
            showAlpha(showAlpha)
        {}

        Color defaultValue = Color(1, 1, 1, 1);
        bool showAlpha = true;
};
