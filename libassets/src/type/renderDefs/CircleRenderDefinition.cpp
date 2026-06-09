//
// Created by droc101 on 6/2/26.
//

#include <cstdint>
#include <libassets/type/Actor.h>
#include <libassets/type/Color.h>
#include <libassets/type/renderDefs/CircleRenderDefinition.h>
#include <libassets/type/renderDefs/RenderDefinition.h>
#include <libassets/type/renderDefs/values/NumericDefinitionValue.h>

CircleRenderDefinition::CircleRenderDefinition(const nlohmann::json &json): RenderDefinition(json)
{
    color = ColorDefinitionValue(json, "color", Color(0, 1, 0, 1));
    radius = NumericDefinitionValue<float>(json, "radius", 1.0f);
    sides = NumericDefinitionValue<int32_t>(json, "num_sides", 16);
}

Color CircleRenderDefinition::GetColor(const Actor &actor)
{
    return color.Get(actor.params, Color(0, 1, 0, 1));
}

float CircleRenderDefinition::GetRadius(const Actor &actor)
{
    return radius.Get(actor.params, 1.0f);
}

int32_t CircleRenderDefinition::GetNumSides(const Actor &actor)
{
    return sides.Get(actor.params, 16);
}
