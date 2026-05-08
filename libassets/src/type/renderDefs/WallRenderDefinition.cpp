//
// Created by droc101 on 4/29/26.
//

#include <libassets/type/Actor.h>
#include <libassets/type/Color.h>
#include <libassets/type/RenderDefinitionValue.h>
#include <libassets/type/renderDefs/WallRenderDefinition.h>
#include <string>

WallRenderDefinition::WallRenderDefinition(const nlohmann::json &json)
{
    color = RenderDefinitionValue<Color>(json, "color", Color(0, 1, 0, 1));
    zAxisOrientation = RenderDefinitionValue<bool>(json, "z_axis_orientation", false);
    localCenterX = RenderDefinitionValue<float>(json, "local_center_x", 0.0f);
    localCenterY = RenderDefinitionValue<float>(json, "local_center_y", 0.0f);
    width = RenderDefinitionValue<float>(json, "width", 1.0f);
    height = RenderDefinitionValue<float>(json, "height", 1.0f);
}

Color WallRenderDefinition::GetColor(const Actor &actor) const
{
    return color.Get(actor.params, Color(0, 1, 0, 1));
}

bool WallRenderDefinition::GetZAxisOrientation(const Actor &actor) const
{
    return zAxisOrientation.Get(actor.params, false);
}

float WallRenderDefinition::GetLocalCenterX(const Actor &actor) const
{
    return localCenterX.Get(actor.params, 0.0f);
}

float WallRenderDefinition::GetLocalCenterY(const Actor &actor) const
{
    return localCenterY.Get(actor.params, 0.0f);
}

glm::vec2 WallRenderDefinition::GetLocalCenter(const Actor &actor) const
{
    return {GetLocalCenterX(actor), GetLocalCenterY(actor)};
}

float WallRenderDefinition::GetWidth(const Actor &actor) const
{
    return width.Get(actor.params, 1.0f);
}

float WallRenderDefinition::GetHeight(const Actor &actor) const
{
    return height.Get(actor.params, 1.0f);
}

glm::vec2 WallRenderDefinition::GetSize(const Actor &actor) const
{
    return {GetWidth(actor), GetHeight(actor)};
}
