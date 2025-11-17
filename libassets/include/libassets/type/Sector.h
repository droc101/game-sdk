//
// Created by droc101 on 9/5/25.
//

#pragma once

#include <array>
#include <cstdint>
#include <libassets/type/Color.h>
#include <libassets/type/WallMaterial.h>
#include <nlohmann/json.hpp>
#include <vector>

class Sector
{
    public:
        Sector() = default;
        explicit Sector(nlohmann::ordered_json j);

        std::vector<std::array<float, 2>> points{};
        std::vector<WallMaterial> wallMaterials{};
        float floorHeight = -1.0;
        float ceilingHeight = 1.0;
        WallMaterial floorMaterial{};
        WallMaterial ceilingMaterial{};
        Color lightColor = Color(-1);

        /// Expensive!
        [[nodiscard]] bool IsValid() const;

        [[nodiscard]] bool ContainsPoint(std::array<float, 2> point) const;

        [[nodiscard]] nlohmann::ordered_json GenerateJson() const;

        [[nodiscard]] std::array<float, 2> SegmentNormal(int segmentIndex) const;

    private:
        enum class SegmentOrientation : uint8_t
        {
            COLINEAR = 0,
            CLOCKWISE = 1,
            COUNTERCLOCKWISE = 2
        };

        [[nodiscard]] static SegmentOrientation GetOrientation(const std::array<float, 2> &pointA,
                                                 const std::array<float, 2> &pointB,
                                                 const std::array<float, 2> &pointC);

        [[nodiscard]] static bool OnSegment(const std::array<float, 2> &segment_start,
                              const std::array<float, 2> &point,
                              const std::array<float, 2> &segment_end);

        [[nodiscard]] static bool CheckIntersection(const std::array<float, 2> &segmentAStart,
                                      const std::array<float, 2> &segmentAEnd,
                                      const std::array<float, 2> &segmentBStart,
                                      const std::array<float, 2> &segmentBEnd);

        [[nodiscard]] double CalculateArea() const;
};
