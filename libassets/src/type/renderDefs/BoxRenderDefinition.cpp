//
// Created by droc101 on 4/28/26.
//

#include <libassets/type/Actor.h>
#include <libassets/type/Color.h>
#include <libassets/type/renderDefs/BoxRenderDefinition.h>
#include <libassets/type/renderDefs/RenderDefinition.h>
#include <libassets/type/renderDefs/values/NumericDefinitionValue.h>

BoxRenderDefinition::BoxRenderDefinition(const nlohmann::json &json): RenderDefinition(json)
{
    color = ColorDefinitionValue(json, "color", Color(0, 1, 0, 1));
    width = NumericDefinitionValue<float>(json, "width", 1.0f);
    height = NumericDefinitionValue<float>(json, "height", 1.0f);
    depth = NumericDefinitionValue<float>(json, "depth", 1.0f);
}

Color BoxRenderDefinition::GetColor(const Actor &actor)
{
    return color.Get(actor.params, Color(0, 1, 0, 1));
}

glm::vec3 BoxRenderDefinition::GetExtents(const Actor &actor)
{
    return {GetWidth(actor), GetHeight(actor), GetDepth(actor)};
}

float BoxRenderDefinition::GetWidth(const Actor &actor)
{
    return width.Get(actor.params, 1.0f);
}

float BoxRenderDefinition::GetHeight(const Actor &actor)
{
    return height.Get(actor.params, 1.0f);
}

float BoxRenderDefinition::GetDepth(const Actor &actor)
{
    return depth.Get(actor.params, 1.0f);
}
