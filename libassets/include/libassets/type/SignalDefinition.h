//
// Created by droc101 on 10/18/25.
//

#pragma once

#include <libassets/type/Param.h>
#include <string>

class SignalDefinition
{
    public:
        SignalDefinition() = default;
        SignalDefinition(const std::string &description, Param::ParamType type);

        [[nodiscard]] const std::string &GetDescription() const;
        [[nodiscard]] Param::ParamType GetType() const;

    private:
        std::string description;
        Param::ParamType paramType = Param::ParamType::PARAM_TYPE_NONE;
};
