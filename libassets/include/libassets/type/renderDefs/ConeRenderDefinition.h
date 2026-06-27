//
// Created by droc101 on 6/27/26.
//

#pragma once

#include <cstdint>
#include <libassets/type/Actor.h>
#include <libassets/type/Color.h>
#include <libassets/type/renderDefs/RenderDefinition.h>
#include <libassets/type/renderDefs/values/ColorDefinitionValue.h>
#include <libassets/type/renderDefs/values/NumericDefinitionValue.h>

class ConeRenderDefinition: public RenderDefinition
{
    public:
        explicit ConeRenderDefinition(const nlohmann::json &json);

        [[nodiscard]] Color GetColor(const Actor &actor);
        [[nodiscard]] float GetLength(const Actor &actor);
        [[nodiscard]] int32_t GetNumSides(const Actor &actor);
        [[nodiscard]] float GetAngle(const Actor &actor);

    private:
        ColorDefinitionValue color;
        NumericDefinitionValue<float> length;
        NumericDefinitionValue<int32_t> sides;
        NumericDefinitionValue<float> angle;
};
