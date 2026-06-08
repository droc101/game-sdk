//
// Created by droc101 on 6/2/26.
//

#pragma once

#include <cstdint>
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
        [[nodiscard]] uint32_t GetNumSides(const Actor &actor);

    private:
        RenderDefinitionValue<Color> color;
        RenderDefinitionValue<float> radius;
        RenderDefinitionValue<uint32_t> sides;
};
