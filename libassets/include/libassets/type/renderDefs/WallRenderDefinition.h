//
// Created by droc101 on 4/29/26.
//

#pragma once

#include <libassets/type/Color.h>
#include <libassets/type/RenderDefinitionValue.h>
#include <libassets/type/renderDefs/RenderDefinition.h>

class WallRenderDefinition: public RenderDefinition
{
    public:
        explicit WallRenderDefinition(const nlohmann::json &json);

        [[nodiscard]] Color GetColor(const Actor &actor) const;
        [[nodiscard]] bool GetZAxisOrientation(const Actor &actor) const;

        [[nodiscard]] float GetLocalCenterX(const Actor &actor) const;
        [[nodiscard]] float GetLocalCenterY(const Actor &actor) const;
        [[nodiscard]] glm::vec2 GetLocalCenter(const Actor &actor) const;

        [[nodiscard]] float GetWidth(const Actor &actor) const;
        [[nodiscard]] float GetHeight(const Actor &actor) const;
        [[nodiscard]] glm::vec2 GetSize(const Actor &actor) const;

    private:
        RenderDefinitionValue<Color> color;
        RenderDefinitionValue<bool> zAxisOrientation;

        RenderDefinitionValue<float> localCenterX;
        RenderDefinitionValue<float> localCenterY;

        RenderDefinitionValue<float> width;
        RenderDefinitionValue<float> height;
};
