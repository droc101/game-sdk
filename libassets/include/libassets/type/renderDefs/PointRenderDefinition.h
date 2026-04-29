//
// Created by droc101 on 4/28/26.
//

#pragma once

#include <libassets/type/Color.h>
#include <libassets/type/RenderDefinitionValue.h>
#include <libassets/type/renderDefs/RenderDefinition.h>

class PointRenderDefinition: public RenderDefinition
{
    public:
        PointRenderDefinition();
        explicit PointRenderDefinition(const nlohmann::json &json);

        [[nodiscard]] Color GetColor(const Actor &actor) const;
        [[nodiscard]] float GetPointSize(const Actor &actor) const;

    private:
        RenderDefinitionValue<Color> color;
        RenderDefinitionValue<float> pointSize;
};
