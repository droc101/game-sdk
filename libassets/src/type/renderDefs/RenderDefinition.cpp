//
// Created by droc101 on 1/9/26.
//

#include <libassets/type/Actor.h>
#include <libassets/type/Color.h>
#include <libassets/type/renderDefs/RenderDefinition.h>
#include <libassets/type/renderDefs/RenderDefinitionValue.h>
#include <string>

RenderDefinition::RenderDefinition(const nlohmann::json &json)
{
    model = RenderDefinitionValue<std::string>(json, "model", "");
    texture = RenderDefinitionValue<std::string>(json, "texture", "");
    color = RenderDefinitionValue<Color>(json, "color", Color(0, 1, 0, 1));
    affectLightmap = RenderDefinitionValue<bool>(json, "affect_lightmap", false);
    directional = RenderDefinitionValue<bool>(json, "directional", true);
    hasBoxRenderer = RenderDefinitionValue<bool>(json, "box", false);
    boxWidth = RenderDefinitionValue<float>(json, "box_width", 1.0f);
    boxHeight = RenderDefinitionValue<float>(json, "box_height", 1.0f);
    boxDepth = RenderDefinitionValue<float>(json, "box_depth", 1.0f);
}

std::string RenderDefinition::GetModel(const Actor &actor) const
{
    return model.Get(actor.params, std::string{"model/error.gmdl"});
}

std::string RenderDefinition::GetTexture(const Actor &actor) const
{
    return texture.Get(actor.params, "");
}

Color RenderDefinition::GetColor(const Actor &actor) const
{
    return color.Get(actor.params, Color(-1));
}

bool RenderDefinition::GetAffectLightmap(const Actor &actor) const
{
    return affectLightmap.Get(actor.params, false);
}

bool RenderDefinition::GetDirectional(const Actor &actor) const
{
    return directional.Get(actor.params, true);
}

bool RenderDefinition::HasBoxRenderer(const Actor &actor) const
{
    return hasBoxRenderer.Get(actor.params, false);
}

glm::vec3 RenderDefinition::GetBoxExtents(const Actor &actor) const
{
    return {
        boxWidth.Get(actor.params, 1.0f),
        boxHeight.Get(actor.params, 1.0f),
        boxDepth.Get(actor.params, 1.0f),
    };
}
