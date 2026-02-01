//
// Created by droc101 on 8/30/25.
//

#include <array>
#include <assimp/config.h>
#include <assimp/Importer.hpp>
#include <assimp/mesh.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <assimp/vector3.h>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <libassets/type/BoundingBox.h>
#include <libassets/type/ConvexHull.h>
#include <libassets/util/DataReader.h>
#include <libassets/util/DataWriter.h>
#include <stdexcept>
#include <string>
#include <vector>

ConvexHull::ConvexHull(DataReader &reader)
{
    const size_t numPoints = reader.Read<size_t>();
    offset = reader.ReadVec3();
    for (size_t i = 0; i < numPoints; i++)
    {
        points.push_back(reader.ReadVec3());
    }
}

ConvexHull::ConvexHull(const std::string &objPath)
{
    Assimp::Importer importer{};
    (void)importer.SetPropertyInteger(AI_CONFIG_PP_RVC_FLAGS,
                                      aiComponent_NORMALS | aiComponent_COLORS | aiComponent_TEXCOORDS);
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

    for (uint32_t i = 0; i < scene->mNumMeshes; i++)
    {
        const aiMesh *mesh = scene->mMeshes[i];

        for (size_t v = 0; v < mesh->mNumVertices; v++)
        {
            const aiVector3d vert = mesh->mVertices[v];
            points.push_back({static_cast<float>(vert.x), static_cast<float>(vert.y), static_cast<float>(vert.z)});
        }
    }

    CalculateOffset();
}

ConvexHull::ConvexHull(const aiMesh *mesh)
{
    for (size_t v = 0; v < mesh->mNumVertices; v++)
    {
        const aiVector3d vert = mesh->mVertices[v];
        points.push_back({static_cast<float>(vert.x), static_cast<float>(vert.y), static_cast<float>(vert.z)});
    }
    CalculateOffset();
}


void ConvexHull::Write(DataWriter &writer) const
{
    writer.Write<size_t>(points.size());
    writer.WriteVec3(offset);
    for (const glm::vec3 &point: points)
    {
        writer.WriteVec3(point);
    }
}

std::vector<glm::vec3> &ConvexHull::GetPoints()
{
    return points;
}

std::vector<float> ConvexHull::GetPointsForRender() const
{
    std::vector<float> buffer{};
    for (const glm::vec3 &point: points)
    {
        buffer.push_back(point.x);
        buffer.push_back(point.y);
        buffer.push_back(point.z);
    }
    return buffer;
}

void ConvexHull::CalculateOffset()
{
    const BoundingBox bb = BoundingBox(points);
    offset = bb.origin;
}

void ConvexHull::ImportMultiple(const std::string &path, std::vector<ConvexHull> &output)
{
    Assimp::Importer importer;
    (void)importer.SetPropertyInteger(AI_CONFIG_PP_RVC_FLAGS,
                                      aiComponent_NORMALS | aiComponent_COLORS | aiComponent_TEXCOORDS);
    const aiScene *scene = importer.ReadFile(path,
                                             aiProcess_JoinIdenticalVertices |
                                                     aiProcess_ValidateDataStructure |
                                                     aiProcess_DropNormals |
                                                     aiProcess_RemoveComponent);

    if (scene == nullptr || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) != 0u || scene->mRootNode == nullptr)
    {
        printf("Assimp error: %s\n", importer.GetErrorString());
        throw std::runtime_error("assimp error, check stdout");
    }

    for (uint32_t i = 0; i < scene->mNumMeshes; i++)
    {
        output.emplace_back(scene->mMeshes[i]);
    }
}
