//
// Created by droc101 on 10/18/25.
//

#pragma once
#include <libassets/type/Param.h>
#include <libassets/util/Error.h>
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

class OptionDefinition
{
    public:
        OptionDefinition() = default;

        static Error::ErrorCode Create(const std::string &path, OptionDefinition &def);

        Param::ParamType GetKeyType() const;

        const Param &GetValue(const std::string &key) const;

        std::vector<std::string> GetOptions() const;

        const std::string &GetName() const;

        std::string Find(const Param &value) const;

    private:
        template<ParamTypeTemplate T> [[nodiscard]] Error::ErrorCode LoadOptions(const nlohmann::json &definitionJson)
        {
            const nlohmann::json &optionsJson = definitionJson.value("options", nlohmann::json{});
            for (const auto &[key, value]: optionsJson.items())
            {
                if constexpr (std::same_as<T, Color>)
                {
                    options[key] = Param(Color(static_cast<uint32_t>(value)));
                } else if constexpr (std::same_as<T, glm::vec2>)
                {
                    options[key] = Param(glm::vec2{
                        value.value("x", 0.0f),
                        value.value("y", 0.0f),
                    });
                } else if constexpr (std::same_as<T, glm::vec3>)
                {
                    options[key] = Param(glm::vec3{
                        value.value("x", 0.0f),
                        value.value("y", 0.0f),
                        value.value("z", 0.0f),
                    });
                } else
                {
                    options[key] = Param(static_cast<T>(value));
                }
            }

            return Error::ErrorCode::OK;
        }

        std::string name;
        Param::ParamType valueType = Param::ParamType::PARAM_TYPE_NONE;
        std::unordered_map<std::string, Param> options{};
};
