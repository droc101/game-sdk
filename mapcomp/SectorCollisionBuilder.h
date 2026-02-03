//
// Created by droc101 on 11/18/25.
//

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <libassets/type/Sector.h>
#include <libassets/util/DataWriter.h>
#include <vector>

class SectorCollisionBuilder
{
    public:
        explicit SectorCollisionBuilder(const Sector &sector);

        void AddWall(size_t wallIndex, float floorHeight, float ceilingHeight);
        void AddFloor();
        void AddCeiling();

        void NextShape();

        void Write(DataWriter &writer);

    private:
        struct SubShape
        {
            public:
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
        void AddSectorBase(bool isFloor);

        void WriteIndex(size_t index, DataWriter &writer, const SubShape &shape) const;

        SubShape &CurrentShape();
};
