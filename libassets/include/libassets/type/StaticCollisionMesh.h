//
// Created by droc101 on 9/1/25.
//

#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <glm/vec3.hpp>
#include <libassets/util/DataReader.h>
#include <libassets/util/DataWriter.h>
#include <string>
#include <vector>

class StaticCollisionMesh
{
    public:
        explicit StaticCollisionMesh(const std::string &objPath);
        explicit StaticCollisionMesh(DataReader &reader);
        StaticCollisionMesh() = default;

        void Write(DataWriter &writer) const;

        [[nodiscard]] std::vector<float> GetVerticesForRender() const;

        [[nodiscard]] size_t GetNumTriangles() const;

    private:
        std::vector<glm::vec3> vertices{};
};
