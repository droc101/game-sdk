//
// Created by droc101 on 9/5/25.
//

#pragma once

#include <array>
#include <nlohmann/json.hpp>
#include <string>

class WallMaterial
{
    public:
        WallMaterial() = default;
        explicit WallMaterial(const std::string &material);
        explicit WallMaterial(nlohmann::ordered_json json);

        std::string material;
        std::array<float, 2> uvOffset{};
        std::array<float, 2> uvScale = {1,1};

        [[nodiscard]] nlohmann::ordered_json GenerateJson() const;
};
