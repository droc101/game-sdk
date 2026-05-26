//
// Created by droc101 on 4/28/26.
//

#pragma once

#include <libassets/type/Actor.h>
#include <libassets/type/Color.h>
#include <libassets/type/RenderDefinitionValue.h>
#include <libassets/type/renderDefs/RenderDefinition.h>

class BoxRenderDefinition: public RenderDefinition
{
    public:
        explicit BoxRenderDefinition(const nlohmann::json &json);

        [[nodiscard]] Color GetColor(const Actor &actor) const;
        [[nodiscard]] glm::vec3 GetExtents(const Actor &actor);
        [[nodiscard]] float GetWidth(const Actor &actor);
        [[nodiscard]] float GetHeight(const Actor &actor);
        [[nodiscard]] float GetDepth(const Actor &actor);

    private:
        RenderDefinitionValue<Color> color;
        RenderDefinitionValue<float> width;
        RenderDefinitionValue<float> height;
        RenderDefinitionValue<float> depth;
};
