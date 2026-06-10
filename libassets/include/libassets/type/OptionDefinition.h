//
// Created by droc101 on 10/18/25.
//

#pragma once
#include <concepts>
#include <cstdint>
#include <libassets/type/Color.h>
#include <libassets/type/Param.h>
#include <libassets/util/Error.h>
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>
#include <vector>

class OptionDefinition
{
    public:
        OptionDefinition() = default;

        static Error::ErrorCode Create(const std::string &path, OptionDefinition &def);

        /**
         * Get the type this option definition uses
         */
        Param::ParamType GetValueType() const;

        /**
         * Get the value of a key
         */
        const Param &GetValue(const std::string &key) const;

        /**
         * Get all keys in this definition
         */
        std::vector<std::string> GetOptions() const;

        /**
         * Get the name of this definition
         */
        const std::string &GetName() const;

        /**
         * Find a value's key
         * @param value The value to search for
         */
        std::string Find(const Param &value) const;

    private:
        std::string name;
        Param::ParamType valueType = Param::ParamType::PARAM_TYPE_NONE;
        std::unordered_map<std::string, Param> options{};

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
};
