//
// Created by droc101 on 4/29/26.
//

#pragma once

#include <libassets/type/Actor.h>
#include <libassets/type/Color.h>
#include <libassets/type/renderDefs/RenderDefinition.h>
#include <libassets/type/renderDefs/values/BoolDefinitionValue.h>
#include <libassets/type/renderDefs/values/ColorDefinitionValue.h>
#include <libassets/type/renderDefs/values/NumericDefinitionValue.h>

class WallRenderDefinition: public RenderDefinition
{
    public:
        explicit WallRenderDefinition(const nlohmann::json &json);

        [[nodiscard]] Color GetColor(const Actor &actor);
        [[nodiscard]] bool GetZAxisOrientation(const Actor &actor);

        [[nodiscard]] float GetLocalCenterX(const Actor &actor);
        [[nodiscard]] float GetLocalCenterY(const Actor &actor);
        [[nodiscard]] glm::vec2 GetLocalCenter(const Actor &actor);

        [[nodiscard]] float GetWidth(const Actor &actor);
        [[nodiscard]] float GetHeight(const Actor &actor);
        [[nodiscard]] glm::vec2 GetSize(const Actor &actor);

    private:
        ColorDefinitionValue color;
        BoolDefinitionValue zAxisOrientation;

        NumericDefinitionValue<float> localCenterX;
        NumericDefinitionValue<float> localCenterY;

        NumericDefinitionValue<float> width;
        NumericDefinitionValue<float> height;
};
