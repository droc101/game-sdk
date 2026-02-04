//
// Created by droc101 on 2/3/26.
//

#pragma once

#include <clipper2/clipper.core.h>
#include <cstdint>
#include <libassets/type/Sector.h>
#include <vector>

class SectorClipper
{
    public:
        explicit SectorClipper(const Sector &sector);

        void AddHole(const Sector *sector);

        void ProcessAndMesh(std::vector<glm::vec2> &vertices, std::vector<uint32_t> &indices) const;

    private:
        Clipper2Lib::PathD polygon{};
        Clipper2Lib::PathsD holes{};
};
