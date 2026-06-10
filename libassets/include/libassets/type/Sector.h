//
// Created by droc101 on 9/5/25.
//

#pragma once

#include <cstdint>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <libassets/type/WallMaterial.h>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

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

        /**
         * Check if this sector is valid
         */
        [[nodiscard]] bool IsValid() const;

        /**
         * Check if this sector contains a 2D point
         */
        [[nodiscard]] bool ContainsPoint(glm::vec2 point) const;

        /**
         * Create a JSON representation of this sector
         * @return
         */
        [[nodiscard]] nlohmann::ordered_json GenerateJson() const;

        /**
         * Get the normal vector of a segment
         * @param segmentIndex The index of the first point of the segment
         */
        [[nodiscard]] glm::vec2 SegmentNormal(size_t segmentIndex) const;

        /**
         * Calculate the area of this sector
         */
        [[nodiscard]] double CalculateArea() const;

        /**
         * Calculate the 3D center position of this sector
         */
        [[nodiscard]] glm::vec3 GetCenter() const;

        /**
         * Calculate the 2D AABB of this sector
         * @return {origin.x, origin.y, extents.x, extents.y}
         */
        [[nodiscard]] glm::vec4 GetAABB() const;

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
