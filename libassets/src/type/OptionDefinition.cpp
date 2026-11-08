//
// Created by droc101 on 10/18/25.
//

#include <cstdint>
#include <libassets/type/ActorDefinition.h>
#include <libassets/type/Color.h>
#include <libassets/type/OptionDefinition.h>
#include <libassets/type/Param.h>
#include <libassets/util/Error.h>
#include <libassets/util/FileIo.h>
#include <libassets/util/Logger.h>
#include <nlohmann/json.hpp>
#include <regex>
#include <string>
#include <utility>
#include <vector>

const Error &OptionDefinition::Create(const std::string &path, OptionDefinition &def)
{
    std::string jsonStr;
    const Error &readError = FileIo::ReadFileToString(path, jsonStr);
    if (readError.Failed())
    {
        return readError;
    }
    const nlohmann::json definitionJson = nlohmann::json::parse(jsonStr);
    if (definitionJson.is_discarded())
    {
        return Errors::INCORRECT_FORMAT;
    }

    def.valueType = Param::ParseType(definitionJson.value("type", std::string()));
    if (def.valueType == Param::ParamType::PARAM_TYPE_NONE)
    {
        Logger::Error("Invalid option value type!");
        return Errors::INCORRECT_FORMAT;
    }

    def.name = std::filesystem::path(path).stem().string();
    if (!std::regex_match(def.name, std::regex(ActorDefinition::VALID_ACTOR_DEFINITION_IDENTIFIER_REGEX)))
    {
        Logger::Error("Invalid options list name (definition file name) \"{}\". Options list names may only contain "
                      "letters and underscores.",
                      def.name);
        return Errors::INCORRECT_FORMAT;
    }

    Error e = Errors::OK;

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
            e = Errors::INCORRECT_FORMAT;
            break;
    }
    if (e.Failed())
    {
        return e;
    }

    return Errors::OK;
}

Param::ParamType OptionDefinition::GetValueType() const
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
