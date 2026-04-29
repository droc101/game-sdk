//
// Created by droc101 on 4/28/26.
//

#include <libassets/type/Actor.h>
#include <libassets/type/Color.h>
#include <libassets/type/RenderDefinitionValue.h>
#include <libassets/type/renderDefs/SpriteRenderDefinition.h>
#include <string>

SpriteRenderDefinition::SpriteRenderDefinition(const nlohmann::json &json)
{
    texture = RenderDefinitionValue<std::string>(json, "texture", "");
    tintColor = RenderDefinitionValue<Color>(json, "tint_color", Color(-1));
    pointSize = RenderDefinitionValue<float>(json, "point_size", 20.0f);
}

std::string SpriteRenderDefinition::GetTexture(const Actor &actor) const
{
    return texture.Get(actor.params, "");
}

Color SpriteRenderDefinition::GetTintColor(const Actor &actor) const
{
    return tintColor.Get(actor.params, Color(-1));
}

float SpriteRenderDefinition::GetPointSize(const Actor &actor) const
{
    return pointSize.Get(actor.params, 20.0f);
}
