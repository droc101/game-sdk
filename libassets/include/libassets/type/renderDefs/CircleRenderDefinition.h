//
// Created by droc101 on 6/2/26.
//

#pragma once

#include <cstdint>
#include <libassets/type/Actor.h>
#include <libassets/type/Color.h>
#include <libassets/type/renderDefs/RenderDefinition.h>
#include <libassets/type/renderDefs/values/ColorDefinitionValue.h>
#include <libassets/type/renderDefs/values/NumericDefinitionValue.h>

class CircleRenderDefinition: public RenderDefinition
{
    public:
        explicit CircleRenderDefinition(const nlohmann::json &json);

        [[nodiscard]] Color GetColor(const Actor &actor);
        [[nodiscard]] float GetRadius(const Actor &actor);
        [[nodiscard]] int32_t GetNumSides(const Actor &actor);

    private:
        ColorDefinitionValue color;
        NumericDefinitionValue<float> radius;
        NumericDefinitionValue<int32_t> sides;
};
