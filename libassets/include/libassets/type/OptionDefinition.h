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
            const std::vector<std::pair<T, std::string>>
                    jsonOptions = definitionJson.value("options", std::vector<std::pair<T, std::string>>());
            for (const std::pair<T, const std::string &> kv: jsonOptions)
            {
                options[kv.second] = Param(kv.first);
            }

            return Error::ErrorCode::OK;
        }

        std::string name;
        Param::ParamType valueType = Param::ParamType::PARAM_TYPE_NONE;
        std::unordered_map<std::string, Param> options{};
};
