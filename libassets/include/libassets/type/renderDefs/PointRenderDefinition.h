//
// Created by droc101 on 4/28/26.
//

#pragma once

#include <libassets/type/Color.h>
#include <libassets/type/renderDefs/RenderDefinition.h>
#include <libassets/type/renderDefs/values/ColorDefinitionValue.h>
#include <libassets/type/renderDefs/values/NumericDefinitionValue.h>

class PointRenderDefinition: public RenderDefinition
{
    public:
        PointRenderDefinition();
        explicit PointRenderDefinition(const nlohmann::json &json);

        [[nodiscard]] Color GetColor(const Actor &actor);
        [[nodiscard]] float GetPointSize(const Actor &actor);

    private:
        ColorDefinitionValue color;
        NumericDefinitionValue<float> pointSize;
};
