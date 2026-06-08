//
// Created by droc101 on 10/18/25.
//

#include <cstdint>
#include <fstream>
#include <libassets/type/ActorDefinition.h>
#include <libassets/type/OptionDefinition.h>
#include <libassets/type/Param.h>
#include <libassets/util/Error.h>
#include <libassets/util/Logger.h>
#include <nlohmann/json.hpp>
#include <regex>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

Error::ErrorCode OptionDefinition::Create(const std::string &path, OptionDefinition &def)
{
    std::ifstream file(path);
    if (!file.is_open())
    {
        return Error::ErrorCode::CANT_OPEN_FILE;
    }
    std::ostringstream ss;
    ss << file.rdbuf();
    const std::string j = ss.str();
    const nlohmann::json definitionJson = nlohmann::json::parse(j);
    if (definitionJson.is_discarded())
    {
        return Error::ErrorCode::INCORRECT_FORMAT;
    }

    def.valueType = Param::ParseType(definitionJson.value("type", std::string()));
    if (def.valueType == Param::ParamType::PARAM_TYPE_NONE)
    {
        Logger::Error("Invalid option value type!");
        return Error::ErrorCode::INCORRECT_FORMAT;
    }

    def.name = std::filesystem::path(path).stem().string();
    if (!std::regex_match(def.name, std::regex(ActorDefinition::VALID_ACTOR_DEFINITION_IDENTIFIER_REGEX)))
    {
        Logger::Error("Invalid options list name (definition file name) \"{}\". Options list names may only contain "
                      "letters and underscores.",
                      def.name);
        return Error::ErrorCode::INCORRECT_FORMAT;
    }

    Error::ErrorCode e = Error::ErrorCode::OK;

    switch (def.valueType)
    {
        case Param::ParamType::PARAM_TYPE_BYTE:
            e = def.LoadOptions<uint8_t>(definitionJson);
            break;
        case Param::ParamType::PARAM_TYPE_INTEGER:
            e = def.LoadOptions<int32_t>(definitionJson);
            break;
        case Param::ParamType::PARAM_TYPE_FLOAT:
            e = def.LoadOptions<float>(definitionJson);
            break;
        case Param::ParamType::PARAM_TYPE_BOOL:
            e = def.LoadOptions<bool>(definitionJson);
            break;
        case Param::ParamType::PARAM_TYPE_STRING:
            e = def.LoadOptions<std::string>(definitionJson);
            break;
        case Param::ParamType::PARAM_TYPE_COLOR:
            e = def.LoadOptions<Color>(definitionJson);
            break;
        case Param::ParamType::PARAM_TYPE_VEC2:
            e = def.LoadOptions<glm::vec2>(definitionJson);
            break;
        case Param::ParamType::PARAM_TYPE_VEC3:
            e = def.LoadOptions<glm::vec3>(definitionJson);
            break;
        default:
            e = Error::ErrorCode::INCORRECT_FORMAT;
            break;
    }
    if (e != Error::ErrorCode::OK)
    {
        return e;
    }

    return Error::ErrorCode::OK;
}

Param::ParamType OptionDefinition::GetKeyType() const
{
    return valueType;
}

const Param &OptionDefinition::GetValue(const std::string &key) const
{
    return options.at(key);
}

std::vector<std::string> OptionDefinition::GetOptions() const
{
    std::vector<std::string> keys{};
    keys.reserve(options.size());
    for (const std::pair<std::string, Param> kv: options)
    {
        keys.push_back(kv.first);
    }
    return keys;
}

const std::string &OptionDefinition::GetName() const
{
    return name;
}

std::string OptionDefinition::Find(const Param &value) const
{
    for (const std::pair<const std::string, Param> &option: options)
    {
        if (option.second == value)
        {
            return option.first;
        }
    }
    return "";
}
