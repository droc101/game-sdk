//
// Created by droc101 on 4/28/26.
//

#pragma once

#include <cstdint>
#include <libassets/type/Color.h>
#include <libassets/type/renderDefs/RenderDefinition.h>
#include <libassets/type/renderDefs/values/BoolDefinitionValue.h>
#include <libassets/type/renderDefs/values/ColorDefinitionValue.h>
#include <libassets/type/renderDefs/values/NumericDefinitionValue.h>
#include <libassets/type/renderDefs/values/StringDefinitionValue.h>
#include <string>

class ModelRenderDefinition: public RenderDefinition
{
    public:
        explicit ModelRenderDefinition(const nlohmann::json &json);

        [[nodiscard]] std::string GetModel(const Actor &actor);
        [[nodiscard]] Color GetColor(const Actor &actor);
        [[nodiscard]] Color GetModColor(const Actor &actor);
        [[nodiscard]] bool GetAffectLightmap(const Actor &actor);
        [[nodiscard]] uint32_t GetSkin(const Actor &actor);

    private:
        StringDefinitionValue model;
        ColorDefinitionValue color;
        ColorDefinitionValue modColor;
        BoolDefinitionValue affectLightmap;
        NumericDefinitionValue<int32_t> skin;
};
