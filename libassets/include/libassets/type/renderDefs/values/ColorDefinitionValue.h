//
// Created by droc101 on 6/9/26.
//

#pragma once

#include <libassets/type/Color.h>
#include <string>
#include "libassets/type/renderDefs/values/NumericDefinitionValue.h"
#include <libassets/type/Param.h>
#include <libassets/type/renderDefs/values/RenderDefinitionValue.h>

class ColorDefinitionValue: public RenderDefinitionValue<Color>
{
    public:
        ColorDefinitionValue() = default;
        explicit ColorDefinitionValue(const Color &color);
        ColorDefinitionValue(const nlohmann::json &json, const std::string &key, const Color &defaultValue);

        Color Get(const KvList &params, const Color &defaultValue) override;

    private:
        bool usesParam = false;
        std::string paramName{};
        NumericDefinitionValue<float> r;
        NumericDefinitionValue<float> g;
        NumericDefinitionValue<float> b;
        NumericDefinitionValue<float> a;

        void SetValue(const Color &color);
};
