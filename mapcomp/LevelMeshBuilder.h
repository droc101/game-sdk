//
// Created by droc101 on 11/16/25.
//

#pragma once

#include <cstddef>
#include <cstdint>
#include <libassets/type/ModelVertex.h>
#include <libassets/type/Sector.h>
#include <libassets/util/DataWriter.h>
#include <string>
#include <vector>

class LevelMeshBuilder
{
    public:
        LevelMeshBuilder() = default;

        void AddWallWithGap(const Sector &sector, size_t wallIndex, float adjFloor, float adjCeil);
        void AddWall(const Sector &sector, size_t wallIndex);
        void AddFloor(const Sector &sector);
        void AddCeiling(const Sector &sector);

        void Write(DataWriter &writer, const std::string &materialPath) const;

    private:
        std::vector<ModelVertex> vertices{};
        std::vector<uint32_t> indices{};
        uint32_t currentIndex = 0;

        static float CalculateSLength(const Sector &sector, size_t wallIndex);
        void AddWallBase(const std::array<float, 2> &startPoint,
                         const std::array<float, 2> &endPoint,
                         const WallMaterial &wallMaterial,
                         std::array<float, 2> wallNormalVector,
                         float previousWallsLength,
                         float floorHeight,
                         float ceilingHeight,
                         const Color &lightColor,
                         bool counterClockWise);
        void AddSectorBase(const Sector &sector, bool isFloor);
};
