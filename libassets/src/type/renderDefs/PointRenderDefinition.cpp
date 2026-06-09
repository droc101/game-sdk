//
// Created by droc101 on 4/28/26.
//

#include <libassets/type/Actor.h>
#include <libassets/type/Color.h>
#include <libassets/type/renderDefs/PointRenderDefinition.h>
#include <libassets/type/renderDefs/RenderDefinition.h>
#include <libassets/type/renderDefs/values/NumericDefinitionValue.h>

PointRenderDefinition::PointRenderDefinition()
{
    color = ColorDefinitionValue(Color(0, 1, 0, 1));
    pointSize = NumericDefinitionValue<float>(10.0f);
    type = RenderDefinitionType::RD_TYPE_POINT;
}

PointRenderDefinition::PointRenderDefinition(const nlohmann::json &json): RenderDefinition(json)
{
    color = ColorDefinitionValue(json, "color", Color(0, 1, 0, 1));
    pointSize = NumericDefinitionValue<float>(json, "point_size", 10.0f);
}

Color PointRenderDefinition::GetColor(const Actor &actor)
{
    return color.Get(actor.params, Color(0, 1, 0, 1));
}

float PointRenderDefinition::GetPointSize(const Actor &actor)
{
    return pointSize.Get(actor.params, 10.0f);
}
