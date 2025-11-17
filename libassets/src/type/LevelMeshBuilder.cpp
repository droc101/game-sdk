//
// Created by droc101 on 11/16/25.
//

#include "LevelMeshBuilder.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <glm/detail/func_geometric.inl>
#include <glm/vec2.hpp>
#include <libassets/asset/LevelMaterialAsset.h>
#include <libassets/type/ModelVertex.h>
#include <libassets/type/Sector.h>
#include <mapbox/earcut.hpp>
#include <vector>

void LevelMeshBuilder::AddCeiling(const Sector &sector)
{
    AddSectorBase(sector, false);
}

void LevelMeshBuilder::AddFloor(const Sector &sector)
{
    AddSectorBase(sector, true);
}

void LevelMeshBuilder::AddWall(const Sector &sector, const size_t wallIndex)
{
    float sLength = 0;
    for (size_t i = 0; i < wallIndex; i++)
    {
        const std::array<float, 2> &startPoint = sector.points.at(i);
        const std::array<float, 2> &endPoint = sector.points.at((i + 1) % (sector.points.size()));
        const glm::vec2 startPointV = {startPoint[0], startPoint[1]};
        const glm::vec2 endPointV = {endPoint[0], endPoint[1]};
        sLength += glm::distance(startPointV, endPointV);
    }

    const WallMaterial &mat = sector.wallMaterials[wallIndex];

    const std::array<float, 2> &startPoint = sector.points.at(wallIndex);
    const std::array<float, 2> &endPoint = sector.points.at((wallIndex + 1) % (sector.points.size()));
    std::array<std::array<float, 3>, 4> wallPoints{};
    wallPoints.at(0) = {startPoint[0], sector.ceilingHeight, startPoint[1]}; // SC
    wallPoints.at(1) = {endPoint[0], sector.ceilingHeight, endPoint[1]}; // EC

    wallPoints.at(2) = {startPoint[0], sector.floorHeight, startPoint[1]}; // SF
    wallPoints.at(3) = {endPoint[0], sector.floorHeight, endPoint[1]}; // EF

    const glm::vec2 startPointV = {startPoint[0], startPoint[1]};
    const glm::vec2 endPointV = {endPoint[0], endPoint[1]};
    const float wallLength = glm::distance(startPointV, endPointV);

    for (const std::array<float, 3> &point: wallPoints)
    {
        ModelVertex v{};
        v.color = sector.lightColor;
        std::array<float, 2> normal = sector.SegmentNormal(wallIndex);
        v.normal[0] = normal[0];
        v.normal[1] = 0;
        v.normal[2] = normal[1];

        v.uv[0] = sLength;
        if (point[0] == endPoint[0] && point[2] == endPoint[1])
        {
            v.uv[0] += wallLength;
        }
        v.uv[1] = -point[1];

        v.uv[0] += mat.uvOffset[0];
        v.uv[1] += mat.uvOffset[1];

        v.uv[0] *= mat.uvScale[0];
        v.uv[1] *= mat.uvScale[1]; // TODO is this the correct way to offset+scale?

        v.position = point;
        vertices.push_back(v);
    }

    indices.push_back(0 + currentIndex);
    indices.push_back(1 + currentIndex);
    indices.push_back(2 + currentIndex);

    indices.push_back(1 + currentIndex);
    indices.push_back(2 + currentIndex);
    indices.push_back(3 + currentIndex);

    currentIndex += 4;
}

void LevelMeshBuilder::AddSectorBase(const Sector &sector, const bool isFloor)
{
    const std::vector<std::vector<std::array<float, 2>>> polygon{sector.points};
    const std::vector<uint32_t> idx = mapbox::earcut<uint32_t>(polygon);

    const WallMaterial &mat = isFloor ? sector.floorMaterial : sector.ceilingMaterial;

    for (const std::array<float, 2> &point: sector.points)
    {
        ModelVertex v{};
        v.color = sector.lightColor;
        v.normal[0] = 0;
        v.normal[2] = 0;
        if (isFloor)
        {
            v.normal[1] = 1;
        } else
        {
            v.normal[1] = -1;
        }
        v.uv = {point.at(0), point.at(1)};
        v.position = {point.at(0), isFloor ? sector.floorHeight : sector.ceilingHeight, point.at(1)};
        v.uv[0] += mat.uvOffset[0];
        v.uv[1] += mat.uvOffset[1];

        v.uv[0] *= mat.uvScale[0];
        v.uv[1] *= mat.uvScale[1]; // TODO is this the correct way to offset+scale?
        vertices.push_back(v);
    }
    for (const uint32_t i: idx)
    {
        indices.push_back(i + currentIndex);
    }
    currentIndex += sector.points.size();
}

void LevelMeshBuilder::Write(DataWriter &writer, const std::string &materialPath) const
{
    writer.WriteString(materialPath);
    writer.Write<uint32_t>(vertices.size());
    for (const ModelVertex &vert: vertices)
    {
        vert.Write(writer);
    }
    writer.Write<uint32_t>(indices.size());
    writer.WriteBuffer<uint32_t>(indices);
}
