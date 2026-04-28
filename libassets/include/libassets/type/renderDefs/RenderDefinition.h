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

        [[nodiscard]] bool GetAffectLightmap(const Actor &actor) const;

        [[nodiscard]] bool HasBoxRenderer(const Actor &actor) const;

        [[nodiscard]] glm::vec3 GetBoxExtents(const Actor &actor) const;

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
        RenderDefinitionValue<bool> affectLightmap;
        RenderDefinitionValue<bool> directional;

        RenderDefinitionValue<bool> hasBoxRenderer;
        RenderDefinitionValue<float> boxWidth;
        RenderDefinitionValue<float> boxHeight;
        RenderDefinitionValue<float> boxDepth;

        static RenderDefinitionValue<std::string> RDV_Str(const nlohmann::json &json,
                                                          const std::string &key,
                                                          const std::string &defaultValue);
        static RenderDefinitionValue<Color> RDV_Color(const nlohmann::json &json,
                                                      const std::string &key,
                                                      Color defaultValue);
        static RenderDefinitionValue<bool> RDV_Bool(const nlohmann::json &json,
                                                    const std::string &key,
                                                    bool defaultValue);
        static RenderDefinitionValue<float> RDV_Float(const nlohmann::json &json,
                                                      const std::string &key,
                                                      float defaultValue);
};
