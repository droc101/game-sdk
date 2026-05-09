//
// Created by droc101 on 4/28/26.
//

#include <libassets/type/Actor.h>
#include <libassets/type/Color.h>
#include <libassets/type/RenderDefinitionValue.h>
#include <libassets/type/renderDefs/PointRenderDefinition.h>

PointRenderDefinition::PointRenderDefinition()
{
    color = RenderDefinitionValue<Color>(Color(0, 1, 0, 1));
    pointSize = RenderDefinitionValue<float>(10.0f);
    type = RenderDefinitionType::RD_TYPE_POINT;
}

PointRenderDefinition::PointRenderDefinition(const nlohmann::json &json)
{
    color = RenderDefinitionValue<Color>(json, "color", Color(0, 1, 0, 1));
    pointSize = RenderDefinitionValue<float>(json, "point_size", 10.0f);
}

Color PointRenderDefinition::GetColor(const Actor &actor) const
{
    return color.Get(actor.params, Color(0, 1, 0, 1));
}

float PointRenderDefinition::GetPointSize(const Actor &actor) const
{
    return pointSize.Get(actor.params, 10.0f);
}
