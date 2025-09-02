//
// Created by droc101 on 9/1/25.
//

#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>
#include <array>
#include <string>
#include <libassets/util/DataReader.h>
#include <libassets/util/DataWriter.h>

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
        std::vector<std::array<float, 3>> vertices{};

};
