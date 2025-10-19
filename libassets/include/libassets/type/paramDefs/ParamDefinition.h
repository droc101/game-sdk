//
// Created by droc101 on 10/18/25.
//

#pragma once

#include <libassets/type/Param.h>
#include <libassets/util/Error.h>
#include <nlohmann/json.hpp>
#include <string>

class ParamDefinition
{
    public:
        virtual ~ParamDefinition() = default;

        Param::ParamType type = Param::ParamType::PARAM_TYPE_NONE;
        std::string description;

        [[nodiscard]] static ParamDefinition *Create(const nlohmann::json &json, Error::ErrorCode &e);
};
