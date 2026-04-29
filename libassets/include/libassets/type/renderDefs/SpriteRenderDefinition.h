//
// Created by droc101 on 4/28/26.
//

#pragma once

#include <libassets/type/Color.h>
#include <libassets/type/RenderDefinitionValue.h>
#include <libassets/type/renderDefs/RenderDefinition.h>
#include <string>

class SpriteRenderDefinition: public RenderDefinition
{
    public:
        explicit SpriteRenderDefinition(const nlohmann::json &json);

        [[nodiscard]] std::string GetTexture(const Actor &actor) const;
        [[nodiscard]] Color GetTintColor(const Actor &actor) const;
        [[nodiscard]] float GetPointSize(const Actor &actor) const;

    private:
        RenderDefinitionValue<std::string> texture;
        RenderDefinitionValue<Color> tintColor;
        RenderDefinitionValue<float> pointSize;
};
