//
// Created by droc101 on 6/27/26.
//

#include <cstdint>
#include <libassets/type/renderDefs/ConeRenderDefinition.h>
#include <libassets/type/renderDefs/RenderDefinition.h>
#include <libassets/type/renderDefs/values/NumericDefinitionValue.h>

ConeRenderDefinition::ConeRenderDefinition(const nlohmann::json &json): RenderDefinition(json)
{
    color = ColorDefinitionValue(json, "color", Color(0, 1, 0, 1));
    length = NumericDefinitionValue<float>(json, "length", 1.0f);
    sides = NumericDefinitionValue<int32_t>(json, "num_sides", 16);
    angle = NumericDefinitionValue<float>(json, "angle", 30.0f);
}

Color ConeRenderDefinition::GetColor(const Actor &actor)
{
    return color.Get(actor.params, Color(0, 1, 0, 1));
}

float ConeRenderDefinition::GetLength(const Actor &actor)
{
    return length.Get(actor.params, 1.0f);
}

int32_t ConeRenderDefinition::GetNumSides(const Actor &actor)
{
    return sides.Get(actor.params, 16);
}

float ConeRenderDefinition::GetAngle(const Actor &actor)
{
    return angle.Get(actor.params, 30.0f);
}
