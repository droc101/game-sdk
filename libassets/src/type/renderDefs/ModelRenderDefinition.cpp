//
// Created by droc101 on 4/28/26.
//

#include <libassets/type/Actor.h>
#include <libassets/type/Color.h>
#include <libassets/type/renderDefs/ModelRenderDefinition.h>
#include <libassets/type/renderDefs/RenderDefinition.h>
#include <string>

ModelRenderDefinition::ModelRenderDefinition(const nlohmann::json &json): RenderDefinition(json)
{
    model = StringDefinitionValue(json, "model", "model/error.gmdl");
    color = ColorDefinitionValue(json, "color", Color(0, 1, 0, 1));
    affectLightmap = BoolDefinitionValue(json, "affect_lightmap", false);
}

std::string ModelRenderDefinition::GetModel(const Actor &actor)
{
    return model.Get(actor.params, "model/error.gmdl");
}

Color ModelRenderDefinition::GetColor(const Actor &actor)
{
    return color.Get(actor.params, Color(0, 1, 0, 1));
}

bool ModelRenderDefinition::GetAffectLightmap(const Actor &actor)
{
    return affectLightmap.Get(actor.params, false);
}
