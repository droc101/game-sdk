//
// Created by droc101 on 10/5/26.
//

#pragma once

#include <string>
#include <libassets/type/Param.h>
#include <nlohmann/json.hpp>


class OutputDefinition
{
public:
    OutputDefinition() = default;
    OutputDefinition(const std::string& description, const Param::ParamType& type);
    explicit OutputDefinition(const nlohmann::json& json);

    /**
     * Get the description of this signal
     */
    [[nodiscard]] const std::string& GetDescription() const;

    /**
     * Get the type of this signal
     */
    [[nodiscard]] Param::ParamType GetType() const;

private:
    std::string description;
    Param::ParamType paramType{};
};
