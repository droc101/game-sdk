//
// Created by droc101 on 11/18/25.
//

#pragma once

#include <libassets/type/Brush.h>
#include <libassets/util/DataWriter.h>
#include <vector>

class BrushCollisionBuilder
{
    public:
        explicit BrushCollisionBuilder(const Brush &brush);

        /**
         * Compile this sector's collision and write it to a DataWriter
         */
        void Write(DataWriter &writer) const;

    private:

        glm::vec3 origin{};
        glm::vec3 rotation{};
        std::vector<glm::vec3> vertices{};
};
