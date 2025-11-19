//
// Created by droc101 on 11/18/25.
//

#pragma once

#include <array>
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

        void NextShape();

        void Write(DataWriter &writer);

    private:
        struct SubShape
        {
            public:
                std::vector<std::array<float, 3>> vertices{};
                std::vector<uint32_t> indices{};
                uint32_t currentIndex = 0;
        };
        const Sector * sector;
        std::array<float, 3> sectorCenter;
        std::vector<SubShape> shapes{};

        void AddWallBase(const std::array<float, 2> &startPoint,
                         const std::array<float, 2> &endPoint,
                         float floorHeight,
                         float ceilingHeight,
                         bool counterClockWise);
        void AddSectorBase(bool isFloor);

        void WriteIndex(size_t index, DataWriter &writer, const SubShape &shape) const;

        SectorCollisionBuilder::SubShape &CurrentShape();
};
