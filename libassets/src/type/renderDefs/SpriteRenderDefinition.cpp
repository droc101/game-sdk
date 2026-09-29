//
// Created by droc101 on 4/28/26.
//

#include <libassets/type/Actor.h>
#include <libassets/type/Color.h>
#include <libassets/type/renderDefs/RenderDefinition.h>
#include <libassets/type/renderDefs/SpriteRenderDefinition.h>
#include <libassets/type/renderDefs/values/NumericDefinitionValue.h>
#include <libassets/util/Colors.h>
#include <string>

SpriteRenderDefinition::SpriteRenderDefinition(const nlohmann::json &json): RenderDefinition(json)
{
    texture = StringDefinitionValue(json, "texture", "");
    tintColor = ColorDefinitionValue(json, "tint_color", Colors::WHITE);
    pointSize = NumericDefinitionValue<float>(json, "point_size", 20.0f);
}

std::string SpriteRenderDefinition::GetTexture(const Actor &actor)
{
    return texture.Get(actor.params, "");
}

Color SpriteRenderDefinition::GetTintColor(const Actor &actor)
{
    return tintColor.Get(actor.params, Colors::WHITE);
}

float SpriteRenderDefinition::GetPointSize(const Actor &actor)
{
    return pointSize.Get(actor.params, 20.0f);
}
