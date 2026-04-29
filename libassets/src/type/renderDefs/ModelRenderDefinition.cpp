//
// Created by droc101 on 4/28/26.
//

#include <libassets/type/Actor.h>
#include <libassets/type/Color.h>
#include <libassets/type/RenderDefinitionValue.h>
#include <libassets/type/renderDefs/ModelRenderDefinition.h>
#include <string>

ModelRenderDefinition::ModelRenderDefinition(const nlohmann::json &json)
{
    model = RenderDefinitionValue<std::string>(json, "model", "model/error.gmdl");
    color = RenderDefinitionValue<Color>(json, "color", Color(0, 1, 0, 1));
    affectLightmap = RenderDefinitionValue<bool>(json, "affect_lightmap", false);
}

std::string ModelRenderDefinition::GetModel(const Actor &actor) const
{
    return model.Get(actor.params, "model/error.gmdl");
}

Color ModelRenderDefinition::GetColor(const Actor &actor) const
{
    return color.Get(actor.params, Color(0, 1, 0, 1));
}

bool ModelRenderDefinition::GetAffectLightmap(const Actor &actor) const
{
    return affectLightmap.Get(actor.params, false);
}
