//
// Created by droc101 on 9/1/25.
//

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <libassets/util/StaticCollisionMesh.h>
#include <assimp/config.h>
#include <assimp/Importer.hpp>
#include <assimp/mesh.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <assimp/vector3.h>
#include <stdexcept>
#include <libassets/util/DataReader.h>
#include <libassets/util/DataWriter.h>
#include <string>
#include <vector>

StaticCollisionMesh::StaticCollisionMesh(const std::string &objPath)
{
    Assimp::Importer importer;
    importer.SetPropertyInteger(AI_CONFIG_PP_RVC_FLAGS,
                                // NOLINT(*-unused-return-value)
                                aiComponent_NORMALS |
                                aiComponent_COLORS |
                                aiComponent_TEXCOORDS);
    const aiScene *scene = importer.ReadFile(objPath,
                                             aiProcess_JoinIdenticalVertices |
                                             aiProcess_ValidateDataStructure |
                                             aiProcess_DropNormals |
                                             aiProcess_RemoveComponent);

    if (scene == nullptr || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) != 0u || scene->mRootNode == nullptr)
    {
        printf("Assimp error: %s\n", importer.GetErrorString());
        throw std::runtime_error("assimp error, check stdout");
    }

    for (uint32_t meshIndex = 0; meshIndex < scene->mNumMeshes; meshIndex++)
    {
        const aiMesh *mesh = scene->mMeshes[meshIndex];

        for (size_t faceIndex = 0; faceIndex < mesh->mNumFaces; faceIndex++)
        {
            const aiFace &face = mesh->mFaces[faceIndex];
            for (uint32_t faceIndexIndex = 0; faceIndexIndex < face.mNumIndices; faceIndexIndex++)
            {
                const uint32_t vertexIndex = face.mIndices[faceIndexIndex];
                const aiVector3d vert = mesh->mVertices[vertexIndex];
                const std::array<float, 3> point = {static_cast<float>(vert.x),
                                                    static_cast<float>(vert.y),
                                                    static_cast<float>(vert.z)};
                vertices.push_back(point);
            }
        }
    }
}

StaticCollisionMesh::StaticCollisionMesh(DataReader &reader)
{
    const size_t numTriangles = reader.Read<size_t>();
    for (size_t i = 0; i < numTriangles; i++)
    {
        for (int v = 0; v < 3; v++)
        {
            const std::array<float, 3> vertex = {
                    reader.Read<float>(),
                    reader.Read<float>(),
                    reader.Read<float>()
            };
            vertices.push_back(vertex);
        }
    }
}

void StaticCollisionMesh::Write(DataWriter &writer) const
{
    writer.Write<size_t>(vertices.size() / 3);
    for (const std::array<float, 3> &vertex: vertices)
    {
        writer.Write<float>(vertex.at(0));
        writer.Write<float>(vertex.at(1));
        writer.Write<float>(vertex.at(2));
    }
}

std::vector<float> StaticCollisionMesh::GetVerticesForRender() const
{
    std::vector<float> points{};
    for (const std::array<float, 3> &vertex: vertices)
    {
        points.push_back(vertex.at(0));
        points.push_back(vertex.at(1));
        points.push_back(vertex.at(2));
    }
    return points;
}

size_t StaticCollisionMesh::GetNumTriangles() const
{
    return vertices.size() / 3;
}
