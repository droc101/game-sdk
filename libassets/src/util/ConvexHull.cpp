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
#include <libassets/util/ConvexHull.h>
#include <libassets/util/DataReader.h>
#include <libassets/util/DataWriter.h>
#include <stdexcept>
#include <string>
#include <vector>
#include "libassets/util/BoundingBox.h"

ConvexHull::ConvexHull(DataReader &reader)
{
    const size_t numPoints = reader.Read<size_t>();
    offset.at(0) = reader.Read<float>();
    offset.at(1) = reader.Read<float>();
    offset.at(2) = reader.Read<float>();
    for (size_t i = 0; i < numPoints; i++)
    {
        std::array<float, 3> point{};
        point.at(0) = reader.Read<float>();
        point.at(1) = reader.Read<float>();
        point.at(2) = reader.Read<float>();
        points.push_back(point);
    }
}

ConvexHull::ConvexHull(const std::string &objPath)
{
    Assimp::Importer importer;
    importer.SetPropertyInteger(AI_CONFIG_PP_RVC_FLAGS, // NOLINT(*-unused-return-value)
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

    for (uint32_t i = 0; i < scene->mNumMeshes; i++)
    {
        const aiMesh *mesh = scene->mMeshes[i];

        for (size_t v = 0; v < mesh->mNumVertices; v++)
        {
            const aiVector3d vert = mesh->mVertices[v];
            const std::array<float, 3> point = {static_cast<float>(vert.x),
                                                static_cast<float>(vert.y),
                                                static_cast<float>(vert.z)};
            points.push_back(point);
        }
    }

    CalculateOffset();
}

ConvexHull::ConvexHull(const aiMesh *mesh)
{
    for (size_t v = 0; v < mesh->mNumVertices; v++)
    {
        const aiVector3d vert = mesh->mVertices[v];
        const std::array<float, 3> point = {static_cast<float>(vert.x),
                                            static_cast<float>(vert.y),
                                            static_cast<float>(vert.z)};
        points.push_back(point);
    }
    CalculateOffset();
}



void ConvexHull::Write(DataWriter &writer) const
{
    writer.Write<size_t>(points.size());
    writer.Write<float>(offset.at(0));
    writer.Write<float>(offset.at(1));
    writer.Write<float>(offset.at(2));
    for (const std::array<float, 3> &point: points)
    {
        writer.WriteBuffer<float>(point);
    }
}

std::vector<std::array<float, 3>> &ConvexHull::GetPoints()
{
    return points;
}

std::vector<float> ConvexHull::GetPointsForRender() const
{
    std::vector<float> buffer{};
    for (const std::array<float, 3> &point: points)
    {
        buffer.push_back(point.at(0));
        buffer.push_back(point.at(1));
        buffer.push_back(point.at(2));
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
    importer.SetPropertyInteger(AI_CONFIG_PP_RVC_FLAGS, // NOLINT(*-unused-return-value)
                                aiComponent_NORMALS |
                                        aiComponent_COLORS |
                                        aiComponent_TEXCOORDS);
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


