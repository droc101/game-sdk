//
// Created by droc101 on 6/2/26.
//

#pragma once

#include <libassets/type/Actor.h>
#include <libassets/type/Color.h>
#include <libassets/type/RenderDefinitionValue.h>
#include <libassets/type/renderDefs/RenderDefinition.h>

class CircleRenderDefinition: public RenderDefinition
{
    public:
        explicit CircleRenderDefinition(const nlohmann::json &json);

        [[nodiscard]] Color GetColor(const Actor &actor) const;
        [[nodiscard]] float GetRadius(const Actor &actor);

    private:
        RenderDefinitionValue<Color> color;
        RenderDefinitionValue<float> radius;
};
