//
// Created by droc101 on 9/5/25.
//

#pragma once

#include <array>
#include <glm/vec2.hpp>
#include <nlohmann/json.hpp>
#include <string>

class WallMaterial
{
    public:
        WallMaterial() = default;
        explicit WallMaterial(const std::string &material);
        explicit WallMaterial(nlohmann::ordered_json json);

        std::string material;
        glm::vec2 uvOffset{};
        glm::vec2 uvScale = {1, 1};

        [[nodiscard]] nlohmann::ordered_json GenerateJson() const;
};
