//
// Created by droc101 on 4/28/26.
//

#pragma once

#include <libassets/type/Actor.h>
#include <libassets/type/Color.h>
#include <libassets/type/renderDefs/RenderDefinition.h>
#include <libassets/type/renderDefs/values/ColorDefinitionValue.h>
#include <libassets/type/renderDefs/values/NumericDefinitionValue.h>

class BoxRenderDefinition: public RenderDefinition
{
    public:
        explicit BoxRenderDefinition(const nlohmann::json &json);

        [[nodiscard]] Color GetColor(const Actor &actor);
        [[nodiscard]] glm::vec3 GetExtents(const Actor &actor);
        [[nodiscard]] float GetWidth(const Actor &actor);
        [[nodiscard]] float GetHeight(const Actor &actor);
        [[nodiscard]] float GetDepth(const Actor &actor);

    private:
        ColorDefinitionValue color;
        NumericDefinitionValue<float> width;
        NumericDefinitionValue<float> height;
        NumericDefinitionValue<float> depth;
};
