//
// Created by droc101 on 9/5/25.
//

#pragma once

#include <array>
#include <cstdint>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <libassets/type/Color.h>
#include <libassets/type/WallMaterial.h>
#include <nlohmann/json.hpp>
#include <vector>

#include "Actor.h"

class Sector
{
    public:
        Sector() = default;
        explicit Sector(nlohmann::ordered_json json);

        std::string name;
        std::vector<glm::vec2> points{};
        std::vector<WallMaterial> wallMaterials{};
        float floorHeight = -1.0;
        float ceilingHeight = 1.0;
        WallMaterial floorMaterial{};
        WallMaterial ceilingMaterial{};
        Color lightColor = Color(-1);

        [[nodiscard]] bool IsValid() const;

        [[nodiscard]] bool ContainsPoint(glm::vec2 point) const;

        [[nodiscard]] nlohmann::ordered_json GenerateJson() const;

        [[nodiscard]] glm::vec2 SegmentNormal(size_t segmentIndex) const;

        [[nodiscard]] double CalculateArea() const;

        [[nodiscard]] glm::vec3 GetCenter() const;

    private:
        enum class SegmentOrientation : uint8_t
        {
            COLINEAR = 0,
            CLOCKWISE = 1,
            COUNTERCLOCKWISE = 2
        };

        [[nodiscard]] static SegmentOrientation GetOrientation(const glm::vec2 &pointA,
                                                               const glm::vec2 &pointB,
                                                               const glm::vec2 &pointC);

        [[nodiscard]] static bool PointsEqual(const glm::vec2 &a, const glm::vec2 &b);

        [[nodiscard]] static bool PointOnSegment(const glm::vec2 &a, const glm::vec2 &b, const glm::vec2 &c);

        [[nodiscard]] static bool CheckIntersection(const glm::vec2 &segmentAStart,
                                                    const glm::vec2 &segmentAEnd,
                                                    const glm::vec2 &segmentBStart,
                                                    const glm::vec2 &segmentBEnd);
};
