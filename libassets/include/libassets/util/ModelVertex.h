//
// Created by droc101 on 7/18/25.
//

#ifndef MODELVERTEX_H
#define MODELVERTEX_H
#include <assimp/mesh.h>
#include <libassets/util/DataReader.h>


class ModelVertex
{
    public:
        ModelVertex() = default;
        explicit ModelVertex(DataReader &reader);
        explicit ModelVertex(const aiMesh *mesh, uint32_t vertexIndex);

        bool operator==(const ModelVertex &other) const;

        std::array<float, 3> position{};
        std::array<float, 3> normal{};
        std::array<float, 2> uv{};
};

template<> struct std::hash<ModelVertex>
{
    size_t operator()(const ModelVertex &vertex) const noexcept;
};

#endif //MODELVERTEX_H
