//
// Created by droc101 on 7/18/25.
//

#pragma once

#include <array>
#include <assimp/mesh.h>
#include <cstddef>
#include <cstdint>
#include <libassets/util/Color.h>
#include <libassets/util/DataReader.h>

class ModelVertex
{
    public:
        ModelVertex() = default;

        explicit ModelVertex(DataReader &reader);

        explicit ModelVertex(const aiMesh *mesh, uint32_t vertexIndex);

        bool operator==(const ModelVertex &other) const;

        std::array<float, 3> position{};

        std::array<float, 2> uv{};

        Color color{};

        std::array<float, 3> normal{};
};

template<> struct std::hash<ModelVertex>
{
    size_t operator()(const ModelVertex &vertex) const noexcept
    {
        constexpr size_t goldenRatio = 0x9e3779b9;
        size_t hashValue = 0;
        for (const float position: vertex.position)
        {
            hashValue ^= std::hash<float>()(position) + goldenRatio + (hashValue << 6) + (hashValue >> 2);
        }
        for (const float normal: vertex.normal)
        {
            hashValue ^= std::hash<float>()(normal) + goldenRatio + (hashValue << 6) + (hashValue >> 2);
        }
        for (const float uv: vertex.uv)
        {
            hashValue ^= std::hash<float>()(uv) + goldenRatio + (hashValue << 6) + (hashValue >> 2);
        }
        for (const float uv: vertex.uv)
        {
            hashValue ^= std::hash<float>()(uv) + goldenRatio + (hashValue << 6) + (hashValue >> 2);
        }
        for (const float color: vertex.color.CopyData())
        {
            hashValue ^= std::hash<float>()(color) + goldenRatio + (hashValue << 6) + (hashValue >> 2);
        }
        return hashValue;
    }
};
