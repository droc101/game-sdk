//
// Created by droc101 on 10/18/25.
//

#include <libassets/type/Param.h>
#include <libassets/type/InputDefinition.h>
#include <string>

const std::string &InputDefinition::GetDescription() const
{
    return description;
}

std::vector<Param::ParamType> InputDefinition::GetTypes() const
{
    return paramTypes;
}

bool InputDefinition::AcceptsType(const Param::ParamType type) const
{
    return std::ranges::find(paramTypes, type) != paramTypes.end();
}

InputDefinition::InputDefinition(const std::string& description, const std::vector<Param::ParamType>& types)
{
    this->description = description;
    paramTypes = types;

    if (paramTypes.empty())
    {
        paramTypes.push_back(Param::ParamType::PARAM_TYPE_NONE);
    }
}

InputDefinition::InputDefinition(const nlohmann::json& json)
{
    description = json.value("description", "");
    if (json.contains("type"))
    {
        paramTypes.push_back(Param::ParseType(json.value("type", "none")));
    } else if (json.contains("types"))
    {
        const std::vector<std::string> types = json.at("types");
        for (const std::string& type : types)
        {
            paramTypes.push_back(Param::ParseType(type));
        }
    }

    if (paramTypes.empty())
    {
        paramTypes.push_back(Param::ParamType::PARAM_TYPE_NONE);
    }
}

Param::ParamType InputDefinition::GetPrimaryType() const
{
    return paramTypes.at(0);
}
