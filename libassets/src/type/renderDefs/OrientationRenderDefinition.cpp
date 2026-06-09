//
// Created by droc101 on 4/28/26.
//

#include <libassets/type/Actor.h>
#include <libassets/type/Color.h>
#include <libassets/type/renderDefs/OrientationRenderDefinition.h>
#include <libassets/type/renderDefs/RenderDefinition.h>

OrientationRenderDefinition::OrientationRenderDefinition()
{
    color = ColorDefinitionValue(Color(0, 1, 0, 1));
    type = RenderDefinitionType::RD_TYPE_ORIENTATION;
}

OrientationRenderDefinition::OrientationRenderDefinition(const nlohmann::json &json): RenderDefinition(json)
{
    color = ColorDefinitionValue(json, "color", Color(0, 1, 0, 1));
}

Color OrientationRenderDefinition::GetColor(const Actor &actor)
{
    return color.Get(actor.params, Color(0, 1, 0, 1));
}
