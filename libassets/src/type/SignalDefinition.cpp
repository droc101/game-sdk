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

Param::ParamType SignalDefinition::GetType() const
{
    return paramType;
}

SignalDefinition::SignalDefinition(const std::string &description, const Param::ParamType type)
{
    this->description = description;
    paramType = type;
}
