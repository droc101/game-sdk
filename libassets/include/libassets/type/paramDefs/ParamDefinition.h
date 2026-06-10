//
// Created by droc101 on 10/18/25.
//

#pragma once

#include <libassets/type/Param.h>
#include <libassets/util/Error.h>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>

class ParamDefinition
{
    public:
        virtual ~ParamDefinition() = default;

        std::string displayName;
        Param::ParamType type = Param::ParamType::PARAM_TYPE_NONE;
        std::string description;

        /**
         * Create a ParamDefinition from JSON
         * @param json The JSON to read from
         * @param e Error code output
         * @param paramName The name of this param
         * @return a std::unique_ptr to the ParamDefinition
         */
        [[nodiscard]] static std::unique_ptr<ParamDefinition> Create(const nlohmann::json &json,
                                                                     Error::ErrorCode &e,
                                                                     const std::string &paramName);
};
