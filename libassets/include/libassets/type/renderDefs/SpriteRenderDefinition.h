//
// Created by droc101 on 4/28/26.
//

#pragma once

#include <libassets/type/Color.h>
#include <libassets/type/renderDefs/RenderDefinition.h>
#include <libassets/type/renderDefs/values/ColorDefinitionValue.h>
#include <libassets/type/renderDefs/values/NumericDefinitionValue.h>
#include <libassets/type/renderDefs/values/StringDefinitionValue.h>
#include <string>

class SpriteRenderDefinition: public RenderDefinition
{
    public:
        explicit SpriteRenderDefinition(const nlohmann::json &json);

        [[nodiscard]] std::string GetTexture(const Actor &actor);
        [[nodiscard]] Color GetTintColor(const Actor &actor);
        [[nodiscard]] float GetPointSize(const Actor &actor);

    private:
        StringDefinitionValue texture;
        ColorDefinitionValue tintColor;
        NumericDefinitionValue<float> pointSize;
};
