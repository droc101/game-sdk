//
// Created by droc101 on 4/28/26.
//

#include <libassets/type/Actor.h>
#include <libassets/type/Color.h>
#include <libassets/type/RenderDefinitionValue.h>
#include <libassets/type/renderDefs/OrientationRenderDefinition.h>

OrientationRenderDefinition::OrientationRenderDefinition()
{
    color = RenderDefinitionValue<Color>(Color(0, 1, 0, 1));
    type = RenderDefinitionType::RD_TYPE_ORIENTATION;
}

OrientationRenderDefinition::OrientationRenderDefinition(const nlohmann::json &json)
{
    color = RenderDefinitionValue<Color>(json, "color", Color(0, 1, 0, 1));
}

Color OrientationRenderDefinition::GetColor(const Actor &actor) const
{
    return color.Get(actor.params, Color(0, 1, 0, 1));
}
