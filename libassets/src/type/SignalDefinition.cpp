//
// Created by droc101 on 10/18/25.
//

#include <libassets/type/Param.h>
#include <libassets/type/SignalDefinition.h>
#include <string>

const std::string &SignalDefinition::GetDescription() const
{
    return description;
}

std::vector<Param::ParamType> SignalDefinition::GetTypes() const
{
    return paramTypes;
}

bool SignalDefinition::AcceptsType(const Param::ParamType type) const
{
    return std::find(paramTypes.begin(), paramTypes.end(), type) != paramTypes.end();
}

SignalDefinition::SignalDefinition(const std::string& description, const std::vector<Param::ParamType>& types)
{
    this->description = description;
    paramTypes = types;

    if (paramTypes.empty())
    {
        paramTypes.push_back(Param::ParamType::PARAM_TYPE_NONE);
    }
}

SignalDefinition::SignalDefinition(const nlohmann::json& json)
{
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

Param::ParamType SignalDefinition::GetPrimaryType() const
{
    return paramTypes.at(0);
}
