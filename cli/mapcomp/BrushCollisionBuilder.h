//
// Created by droc101 on 11/18/25.
//

#pragma once

#include <cstddef>
#include <cstdint>
#include <libassets/type/Brush.h>
#include <libassets/util/DataWriter.h>
#include <vector>

class BrushCollisionBuilder
{
    public:
        explicit BrushCollisionBuilder(const Brush &brush);

        void AddFace(size_t faceIndex);

        /**
         * Compile this sector's collision and write it to a DataWriter
         */
        void Write(DataWriter &writer);

    private:

        glm::vec3 origin;
        glm::vec3 rotation;
        std::vector<uint32_t> indices{};
        std::vector<glm::vec3> vertices{};

        const Brush *currentBrush = nullptr;
};
