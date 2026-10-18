//
// Created by droc101 on 2/3/26.
//

#pragma once

#include <clipper2/clipper.core.h>
#include <cstdint>
#include <libassets/type/Brush.h>
#include <vector>

class SectorClipper
{
    public:
        explicit SectorClipper(const Brush &sector, size_t faceIndex);

        void AddHole(const Brush &brush, size_t faceIndex);

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
