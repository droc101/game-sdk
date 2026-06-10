//
// Created by droc101 on 9/1/25.
//

#pragma once

#include <cstddef>
#include <glm/vec3.hpp>
#include <libassets/util/DataReader.h>
#include <libassets/util/DataWriter.h>
#include <libassets/util/Error.h>
#include <string>
#include <vector>

class StaticCollisionMesh
{
    public:
        StaticCollisionMesh(const std::string &objPath, Error::ErrorCode &status);
        explicit StaticCollisionMesh(DataReader &reader);
        StaticCollisionMesh() = default;

        /**
         * Write this StaticCollisionMesh to a DataWriter
         */
        void Write(DataWriter &writer) const;

        /**
         * Get the vertices of this StaticCollisionMesh for rendering
         */
        [[nodiscard]] std::vector<float> GetVerticesForRender() const;

        /**
         * Get the number of triangles in this mesh
         */
        [[nodiscard]] size_t GetNumTriangles() const;

    private:
        std::vector<glm::vec3> vertices{};
};
