//
// Created by droc101 on 4/28/26.
//

#pragma once

#include <libassets/type/Color.h>
#include <libassets/type/renderDefs/RenderDefinition.h>
#include <libassets/type/renderDefs/values/ColorDefinitionValue.h>
#include <libassets/type/renderDefs/values/StringDefinitionValue.h>
#include <string>

class ModelRenderDefinition: public RenderDefinition
{
    public:
        explicit ModelRenderDefinition(const nlohmann::json &json);

        [[nodiscard]] std::string GetModel(const Actor &actor);
        [[nodiscard]] Color GetColor(const Actor &actor);
        [[nodiscard]] bool GetAffectLightmap(const Actor &actor);

    private:
        StringDefinitionValue model;
        ColorDefinitionValue color;
        BoolDefinitionValue affectLightmap;
};
