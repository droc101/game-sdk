//
// Created by droc101 on 1/8/26.
//

#pragma once

#include <libassets/type/Color.h>
#include <nlohmann/json.hpp>
#include <string>

class Actor;

class RenderDefinition
{
    public:
        RenderDefinition() = default;

        explicit RenderDefinition(const nlohmann::json &json);

        [[nodiscard]] std::string GetModel(const Actor &actor) const;

        [[nodiscard]] Color GetColor(const Actor &actor) const;

        [[nodiscard]] std::string GetTexture(const Actor &actor) const;

        [[nodiscard]] bool GetDirectional(const Actor &actor) const;

    private:
        template<typename T> struct RenderDefinitionValue
        {
                bool usesParam = false;
                std::string paramName;
                T value{};
        };

        RenderDefinitionValue<std::string> model;
        RenderDefinitionValue<Color> color;
        RenderDefinitionValue<std::string> texture;
        RenderDefinitionValue<bool> directional;
};
