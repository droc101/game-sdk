//
// Created by droc101 on 11/18/25.
//

#pragma once

#include <cstddef>
#include <cstdint>
#include <libassets/type/Sector.h>
#include <libassets/util/DataWriter.h>
#include <vector>

class SectorCollisionBuilder
{
    public:
        /**
         * Create a collision builder for a sector
         * @param sector The sector to build collision for
         */
        explicit SectorCollisionBuilder(const Sector &sector);

        /**
         * Add a wall
         * @param wallIndex The index of the wall
         * @param floorHeight The bottom of the wall
         * @param ceilingHeight The top of the wall
         * @note The @c floorHeight and @c ceilingHeight do NOT have to match the sector's floor and ceiling heights.
         */
        void AddWall(size_t wallIndex, float floorHeight, float ceilingHeight);

        /**
         * Add a floor for this sector
         * @param overlapping A list of overlapping sectors to cut holes for
         */
        void AddFloor(const std::vector<const Sector *> &overlapping);

        /**
         * Add a ceiling for this sector
         * @param overlapping A list of overlapping sectors to cut holes for
         */
        void AddCeiling(const std::vector<const Sector *> &overlapping);

        /**
         * Create a new sub shape for future calls
         */
        void NextShape();

        /**
         * Compile this sector's collision and write it to a DataWriter
         */
        void Write(DataWriter &writer);

    private:
        struct SubShape
        {
                std::vector<glm::vec3> vertices{};
                std::vector<uint32_t> indices{};
                uint32_t currentIndex = 0;
        };
        const Sector *sector;
        glm::vec3 sectorCenter;
        std::vector<SubShape> shapes{};

        void AddWallBase(const glm::vec2 &startPoint,
                         const glm::vec2 &endPoint,
                         float floorHeight,
                         float ceilingHeight,
                         bool counterClockWise);
        void AddSectorBase(bool isFloor, const std::vector<const Sector *> &overlapping);

        void WriteIndex(size_t index, DataWriter &writer, const SubShape &shape) const;

        SubShape &CurrentShape();
};
