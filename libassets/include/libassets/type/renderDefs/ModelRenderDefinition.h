//
// Created by droc101 on 4/28/26.
//

#pragma once

#include <libassets/type/Color.h>
#include <libassets/type/RenderDefinitionValue.h>
#include <libassets/type/renderDefs/RenderDefinition.h>
#include <string>

class ModelRenderDefinition: public RenderDefinition
{
    public:
        explicit ModelRenderDefinition(const nlohmann::json &json);

        [[nodiscard]] std::string GetModel(const Actor &actor) const;
        [[nodiscard]] Color GetColor(const Actor &actor) const;
        [[nodiscard]] bool GetAffectLightmap(const Actor &actor) const;

    private:
        RenderDefinitionValue<std::string> model;
        RenderDefinitionValue<Color> color;
        RenderDefinitionValue<bool> affectLightmap;
};
