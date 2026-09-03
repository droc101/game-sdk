//
// Created by droc101 on 9/2/26.
//

#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <glm/fwd.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <libassets/type/BoundingBox.h>
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

class Brush
{
    public:
        class Face
        {
            public:
                Face() = default;
                explicit Face(nlohmann::ordered_json json);

                [[nodiscard]] nlohmann::ordered_json GenerateJson() const;

                std::vector<uint32_t> indices{};

                std::string material;

                glm::vec2 textureScale = {1.0f, 1.0f};
                glm::vec2 textureOffset = {0.0f, 0.0f};
                float textureRotation = 0;

                float unitsPerLuxel = 1.0f;
        };

        Brush() = default;
        explicit Brush(nlohmann::ordered_json json);

        [[nodiscard]] nlohmann::ordered_json GenerateJson() const;

        [[nodiscard]] bool IsValid() const;

        [[nodiscard]] BoundingBox GetAABB() const;

        [[nodiscard]] glm::mat4 GetTransformMatrix() const;

        [[nodiscard]] std::unordered_set<std::pair<glm::vec3, glm::vec3>> GetUniqueEdges() const;

        std::string editorName{};

        glm::vec3 origin{};
        glm::vec3 rotation{};

        std::vector<glm::vec3> vertices{};
        std::vector<Face> faces{};
};

template<> struct std::hash<std::pair<glm::vec3, glm::vec3>>
{
    size_t operator()(const std::pair<glm::vec3, glm::vec3> &pair) const noexcept
    {
        constexpr size_t GOLDEN_RATIO = 0x9e3779b9;
        size_t hashValue = 0;

        hashValue ^= std::hash<float>()(pair.first.x) + GOLDEN_RATIO + (hashValue << 6) + (hashValue >> 2);
        hashValue ^= std::hash<float>()(pair.first.y) + GOLDEN_RATIO + (hashValue << 6) + (hashValue >> 2);
        hashValue ^= std::hash<float>()(pair.first.z) + GOLDEN_RATIO + (hashValue << 6) + (hashValue >> 2);

        hashValue ^= std::hash<float>()(pair.second.x) + GOLDEN_RATIO + (hashValue << 6) + (hashValue >> 2);
        hashValue ^= std::hash<float>()(pair.second.y) + GOLDEN_RATIO + (hashValue << 6) + (hashValue >> 2);
        hashValue ^= std::hash<float>()(pair.second.z) + GOLDEN_RATIO + (hashValue << 6) + (hashValue >> 2);

        return hashValue;
    }
};
