//
// Created by droc101 on 10/5/26.
//

#include <libassets/type/OutputDefinition.h>

OutputDefinition::OutputDefinition(const nlohmann::json& json)
{
    description = json.value("description", "");
    paramType = Param::ParseType(json.value("type", "none"));
}

OutputDefinition::OutputDefinition(const std::string& description, const Param::ParamType& type)
{
    this->description = description;
    paramType = type;
}

const std::string& OutputDefinition::GetDescription() const
{
    return description;
}

Param::ParamType OutputDefinition::GetType() const
{
    return paramType;
}
