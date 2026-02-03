//
// Created by droc101 on 7/18/25.
//

#pragma once

#include <array>
#include <assimp/mesh.h>
#include <cstddef>
#include <cstdint>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <libassets/type/Color.h>
#include <libassets/util/DataReader.h>

class ModelVertex
{
    public:
        ModelVertex() = default;

        explicit ModelVertex(DataReader &reader);

        explicit ModelVertex(const aiMesh *mesh, uint32_t vertexIndex);

        bool operator==(const ModelVertex &other) const;

        void Write(DataWriter &writer) const;

        glm::vec3 position{};

        glm::vec2 uv{};

        Color color{};

        glm::vec3 normal{};
};

template<> struct std::hash<ModelVertex>
{
        size_t operator()(const ModelVertex &vertex) const noexcept
        {
            constexpr size_t goldenRatio = 0x9e3779b9;
            size_t hashValue = 0;

            hashValue ^= std::hash<float>()(vertex.position.x) + goldenRatio + (hashValue << 6) + (hashValue >> 2);
            hashValue ^= std::hash<float>()(vertex.position.y) + goldenRatio + (hashValue << 6) + (hashValue >> 2);
            hashValue ^= std::hash<float>()(vertex.position.z) + goldenRatio + (hashValue << 6) + (hashValue >> 2);

            hashValue ^= std::hash<float>()(vertex.normal.x) + goldenRatio + (hashValue << 6) + (hashValue >> 2);
            hashValue ^= std::hash<float>()(vertex.normal.y) + goldenRatio + (hashValue << 6) + (hashValue >> 2);
            hashValue ^= std::hash<float>()(vertex.normal.z) + goldenRatio + (hashValue << 6) + (hashValue >> 2);

            hashValue ^= std::hash<float>()(vertex.uv.x) + goldenRatio + (hashValue << 6) + (hashValue >> 2);
            hashValue ^= std::hash<float>()(vertex.uv.y) + goldenRatio + (hashValue << 6) + (hashValue >> 2);

            for (const float color: vertex.color.CopyData())
            {
                hashValue ^= std::hash<float>()(color) + goldenRatio + (hashValue << 6) + (hashValue >> 2);
            }
            return hashValue;
        }
};
