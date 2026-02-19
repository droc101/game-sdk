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
        /**
         * Create a SectorClipper for a sector
         */
        explicit SectorClipper(const Sector &sector);

        /**
         * Add a sector to use as a hole
         * @param sector The sector to cut a hole for
         */
        void AddHole(const Sector *sector);

        /**
         * Perform the clipping process and create mesh data
         * @param vertices Output vertices
         * @param indices Output indices
         */
        void ProcessAndMesh(std::vector<glm::vec2> &vertices, std::vector<uint32_t> &indices) const;

    private:
        Clipper2Lib::PathD polygon{};
        Clipper2Lib::PathsD holes{};
};
