//
// Created by droc101 on 4/29/26.
//

#include <libassets/type/Actor.h>
#include <libassets/type/Color.h>
#include <libassets/type/renderDefs/RenderDefinition.h>
#include <libassets/type/renderDefs/values/NumericDefinitionValue.h>
#include <libassets/type/renderDefs/WallRenderDefinition.h>
#include <string>

WallRenderDefinition::WallRenderDefinition(const nlohmann::json &json): RenderDefinition(json)
{
    color = ColorDefinitionValue(json, "color", Color(0, 1, 0, 1));
    zAxisOrientation = BoolDefinitionValue(json, "z_axis_orientation", false);
    localCenterX = NumericDefinitionValue<float>(json, "local_center_x", 0.0f);
    localCenterY = NumericDefinitionValue<float>(json, "local_center_y", 0.0f);
    width = NumericDefinitionValue<float>(json, "width", 1.0f);
    height = NumericDefinitionValue<float>(json, "height", 1.0f);
    texture = StringDefinitionValue(json, "texture", "");
    tintColor = ColorDefinitionValue(json, "tint_color", Color(-1));
    uvOffsetX = NumericDefinitionValue<float>(json, "uv_offset_x", 0.0f);
    uvOffsetY = NumericDefinitionValue<float>(json, "uv_offset_y", 0.0f);
    uvScaleX = NumericDefinitionValue<float>(json, "uv_scale_x", 1.0f);
    uvScaleY = NumericDefinitionValue<float>(json, "uv_scale_y", 1.0f);
    unshaded = BoolDefinitionValue(json, "unshaded", false);
}

Color WallRenderDefinition::GetColor(const Actor &actor)
{
    return color.Get(actor.params, Color(0, 1, 0, 1));
}

bool WallRenderDefinition::GetZAxisOrientation(const Actor &actor)
{
    return zAxisOrientation.Get(actor.params, false);
}

float WallRenderDefinition::GetLocalCenterX(const Actor &actor)
{
    return localCenterX.Get(actor.params, 0.0f);
}

float WallRenderDefinition::GetLocalCenterY(const Actor &actor)
{
    return localCenterY.Get(actor.params, 0.0f);
}

glm::vec2 WallRenderDefinition::GetLocalCenter(const Actor &actor)
{
    return {GetLocalCenterX(actor), GetLocalCenterY(actor)};
}

float WallRenderDefinition::GetWidth(const Actor &actor)
{
    return width.Get(actor.params, 1.0f);
}

float WallRenderDefinition::GetHeight(const Actor &actor)
{
    return height.Get(actor.params, 1.0f);
}

glm::vec2 WallRenderDefinition::GetSize(const Actor &actor)
{
    return {GetWidth(actor), GetHeight(actor)};
}

std::string WallRenderDefinition::GetTexture(const Actor &actor)
{
    return texture.Get(actor.params, "");
}

Color WallRenderDefinition::GetTintColor(const Actor &actor)
{
    return tintColor.Get(actor.params, Color(-1));
}

float WallRenderDefinition::GetUvOffsetX(const Actor &actor)
{
    return uvOffsetX.Get(actor.params, 0.0f);
}

float WallRenderDefinition::GetUvOffsetY(const Actor &actor)
{
    return uvOffsetY.Get(actor.params, 0.0f);
}

glm::vec2 WallRenderDefinition::GetUvOffset(const Actor &actor)
{
    return {GetUvOffsetX(actor), GetUvOffsetY(actor)};
}

float WallRenderDefinition::GetUvScaleX(const Actor &actor)
{
    return uvScaleX.Get(actor.params, 1.0f);
}

float WallRenderDefinition::GetUvScaleY(const Actor &actor)
{
    return uvScaleY.Get(actor.params, 1.0f);
}

glm::vec2 WallRenderDefinition::GetUvScale(const Actor &actor)
{
    return {GetUvScaleX(actor), GetUvScaleY(actor)};
}

bool WallRenderDefinition::IsUnshaded(const Actor &actor)
{
    return unshaded.Get(actor.params, false);
}
