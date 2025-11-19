//
// Created by droc101 on 11/18/25.
//

#pragma once

#include <cstddef>
#include <cstdint>
#include <libassets/type/Sector.h>
#include <vector>

class SectorCollisionBuilder
{
    public:
        SectorCollisionBuilder(const Sector &sector);

        void AddWallWithGap(size_t wallIndex, float adjFloor, float adjCeil);
        void AddWall(size_t wallIndex);
        void AddFloor();
        void AddCeiling();

        void Write(DataWriter &writer) const;

    private:
        const Sector * sector;
        std::vector<std::array<float, 3>> vertices{};
        std::vector<uint32_t> indices{};
        uint32_t currentIndex = 0;

        void AddWallBase(const std::array<float, 2> &startPoint,
                         const std::array<float, 2> &endPoint,
                         float floorHeight,
                         float ceilingHeight,
                         bool counterClockWise);
        void AddSectorBase(bool isFloor);
};
