//
// Created by droc101 on 6/2/26.
//

#include <cstdint>
#include <libassets/type/Actor.h>
#include <libassets/type/Color.h>
#include <libassets/type/RenderDefinitionValue.h>
#include <libassets/type/renderDefs/CircleRenderDefinition.h>
#include <libassets/type/renderDefs/RenderDefinition.h>

CircleRenderDefinition::CircleRenderDefinition(const nlohmann::json &json): RenderDefinition(json)
{
    color = RenderDefinitionValue<Color>(json, "color", Color(0, 1, 0, 1));
    radius = RenderDefinitionValue<float>(json, "radius", 1.0f);
    sides = RenderDefinitionValue<uint32_t>(json, "num_sides", 16);
}

Color CircleRenderDefinition::GetColor(const Actor &actor) const
{
    return color.Get(actor.params, Color(0, 1, 0, 1));
}

float CircleRenderDefinition::GetRadius(const Actor &actor)
{
    return radius.GetFloat(actor.params);
}

uint32_t CircleRenderDefinition::GetNumSides(const Actor &actor)
{
    return sides.GetInt(actor.params);
}
