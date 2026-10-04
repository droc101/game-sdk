//
// Created by droc101 on 10/18/25.
//

#pragma once

#include <libassets/type/Param.h>
#include <string>
#include <unordered_set>

class SignalDefinition
{
public:
    SignalDefinition() = default;
    SignalDefinition(const std::string& description, const std::vector<Param::ParamType>& types);
    explicit SignalDefinition(const nlohmann::json& json);

    /**
     * Get the description of this signal
     */
    [[nodiscard]] const std::string& GetDescription() const;

    /**
     * Get the types of this signal
     */
    [[nodiscard]] std::vector<Param::ParamType> GetTypes() const;

    [[nodiscard]] Param::ParamType GetPrimaryType() const;

    [[nodiscard]] bool AcceptsType(Param::ParamType type) const;

private:
    std::string description;
    std::vector<Param::ParamType> paramTypes{};
};
