//
// Created by droc101 on 4/28/26.
//

#include <libassets/type/Actor.h>
#include <libassets/type/Color.h>
#include <libassets/type/RenderDefinitionValue.h>
#include <libassets/type/renderDefs/BoxRenderDefinition.h>

BoxRenderDefinition::BoxRenderDefinition(const nlohmann::json &json)
{
    color = RenderDefinitionValue<Color>(json, "color", Color(0, 1, 0, 1));
    width = RenderDefinitionValue<float>(json, "width", 1.0f);
    height = RenderDefinitionValue<float>(json, "height", 1.0f);
    depth = RenderDefinitionValue<float>(json, "depth", 1.0f);
}

Color BoxRenderDefinition::GetColor(const Actor &actor) const
{
    return color.Get(actor.params, Color(0, 1, 0, 1));
}

glm::vec3 BoxRenderDefinition::GetExtents(const Actor &actor) const
{
    return {GetWidth(actor), GetHeight(actor), GetDepth(actor)};
}

float BoxRenderDefinition::GetWidth(const Actor &actor) const
{
    return width.Get(actor.params, 1.0f);
}

float BoxRenderDefinition::GetHeight(const Actor &actor) const
{
    return height.Get(actor.params, 1.0f);
}

float BoxRenderDefinition::GetDepth(const Actor &actor) const
{
    return depth.Get(actor.params, 1.0f);
}
