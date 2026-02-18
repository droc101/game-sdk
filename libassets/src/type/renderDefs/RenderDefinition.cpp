//
// Created by droc101 on 1/9/26.
//

#include <libassets/type/Color.h>
#include <libassets/type/renderDefs/RenderDefinition.h>
#include <libassets/util/Error.h>
#include <string>

RenderDefinition RenderDefinition::Create(const nlohmann::json &json, Error::ErrorCode &e)
{
    RenderDefinition output = RenderDefinition();
    output.modelSourceParam = json.value("model_param", "");
    output.colorSourceParam = json.value("color_param", "");
    output.textureSourceParam = json.value("texture_param", "");
    output.color = Color(json.value("color", 0x00ff00ff));
    output.model = json.value("model", "");
    output.texture = json.value("texture", "");
    output.directional = json.value("directional", true);

    e = Error::ErrorCode::OK;
    return output;
}
