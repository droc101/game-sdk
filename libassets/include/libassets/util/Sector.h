//
// Created by droc101 on 9/5/25.
//

#pragma once

#include <array>
#include <cstdint>
#include <vector>
#include "Color.h"
#include "WallMaterial.h"

class Sector
{
    public:
        std::vector<std::array<float, 2>> points{};
        std::vector<WallMaterial> wallMaterials;
        float floorHeight;
        float ceilingHeight;
        WallMaterial floorMaterial;
        WallMaterial ceilingMaterial;
        Color lightColor;

        /// Expensive!
        bool IsValid();

        bool ContainsPoint(std::array<float, 2> point) const;

    private:
        enum class SegmentOrientation : uint8_t
        {
            COLINEAR = 0,
            CLOCKWISE = 1,
            COUNTERCLOCKWISE = 2
        };

        static SegmentOrientation GetOrientation(const std::array<float, 2> &pointA,
                                                 const std::array<float, 2> &pointB,
                                                 const std::array<float, 2> &pointC);

        static bool OnSegment(const std::array<float, 2> &segment_start,
                              const std::array<float, 2> &point,
                              const std::array<float, 2> &segment_end);

        static bool CheckIntersection(const std::array<float, 2> &segmentAStart,
                                      const std::array<float, 2> &segmentAEnd,
                                      const std::array<float, 2> &segmentBStart,
                                      const std::array<float, 2> &segmentBEnd);
};
