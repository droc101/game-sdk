//
// Created by droc101 on 11/16/25.
//

#pragma once

#include <cstddef>
#include <cstdint>
#include <libassets/type/Color.h>
#include <libassets/type/ModelVertex.h>
#include <libassets/type/Sector.h>
#include <libassets/type/WallMaterial.h>
#include <libassets/util/DataWriter.h>
#include <string>
#include <vector>

class LevelMeshBuilder
{
    public:
        LevelMeshBuilder() = default;

        /**
         * Add a wall
         * @param sector The sector containing the wall
         * @param wallIndex The wall index in the sector
         * @param floorHeight The bottom of the wall
         * @param ceilingHeight The top of the wall
         */
        void AddWall(const Sector &sector, size_t wallIndex, float floorHeight, float ceilingHeight);

        /**
         * Add a floor
         * @param sector The sector to add the floor of
         * @param overlapping A list of overlapping sectors to cut holes for
         */
        void AddFloor(const Sector &sector, const std::vector<const Sector *> &overlapping);

        /**
         * Add a ceiling
         * @param sector The sector to add the ceiling of
         * @param overlapping A list of overlapping sectors to cut holes for
         */
        void AddCeiling(const Sector &sector, const std::vector<const Sector *> &overlapping);

        /**
         * Write this mesh to a DataWriter
         * @param writer The DataWriter to write to
         * @param materialPath The material to use
         */
        void Write(DataWriter &writer, const std::string &materialPath) const;

        /**
         * Check if this builder is empty
         */
        [[nodiscard]] bool IsEmpty() const;

    private:
        std::vector<ModelVertex> vertices{};
        std::vector<uint32_t> indices{};
        uint32_t currentIndex = 0;

        static float CalculateSLength(const Sector &sector, size_t wallIndex);
        void AddWallBase(const glm::vec2 &startPoint,
                         const glm::vec2 &endPoint,
                         const WallMaterial &wallMaterial,
                         glm::vec2 wallNormalVector,
                         float previousWallsLength,
                         float floorHeight,
                         float ceilingHeight,
                         const Color &lightColor,
                         bool counterClockWise);
        void AddSectorBase(const Sector &sector, bool isFloor, const std::vector<const Sector *> &overlapping);
};
