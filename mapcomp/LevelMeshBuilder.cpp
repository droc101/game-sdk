//
// Created by droc101 on 11/16/25.
//

#include "LevelMeshBuilder.h"
#include <array>
#include <cstdint>
#include <cstdio>
#include <glm/detail/func_geometric.inl>
#include <glm/vec2.hpp>
#include <libassets/type/Color.h>
#include <libassets/type/ModelVertex.h>
#include <libassets/type/Sector.h>
#include <libassets/type/WallMaterial.h>
#include <libassets/util/DataWriter.h>
#include <string>
#include <utility>
#include <vector>
#include "SectorClipper.h"

void LevelMeshBuilder::AddCeiling(const Sector &sector, const std::vector<const Sector *> &overlapping)
{
    AddSectorBase(sector, false, overlapping);
}

void LevelMeshBuilder::AddFloor(const Sector &sector, const std::vector<const Sector *> &overlapping)
{
    AddSectorBase(sector, true, overlapping);
}

void LevelMeshBuilder::AddWall(const Sector &sector,
                               const size_t wallIndex,
                               const float floorHeight,
                               const float ceilingHeight)
{
    const float sLength = CalculateSLength(sector, wallIndex);

    const WallMaterial &mat = sector.wallMaterials.at(wallIndex);

    const glm::vec2 &startPoint = sector.points.at(wallIndex);
    const glm::vec2 &endPoint = sector.points.at((wallIndex + 1) % (sector.points.size()));

    const glm::vec2 normal = sector.SegmentNormal(wallIndex);
    const bool ccw = sector.CalculateArea() > 0;

    AddWallBase(startPoint, endPoint, mat, normal, sLength, floorHeight, ceilingHeight, sector.lightColor, ccw);
}

float LevelMeshBuilder::CalculateSLength(const Sector &sector, const size_t wallIndex)
{
    float sLength = 0;
    for (size_t i = 0; i < wallIndex; i++)
    {
        const glm::vec2 &startPoint = sector.points.at(i);
        const glm::vec2 &endPoint = sector.points.at((i + 1) % (sector.points.size()));
        sLength += glm::distance(startPoint, endPoint);
    }
    return sLength;
}


void LevelMeshBuilder::AddWallBase(const glm::vec2 &startPoint,
                                   const glm::vec2 &endPoint,
                                   const WallMaterial &wallMaterial,
                                   const glm::vec2 wallNormalVector,
                                   const float previousWallsLength,
                                   const float floorHeight,
                                   const float ceilingHeight,
                                   const Color &lightColor,
                                   const bool counterClockWise)
{
    if (floorHeight > ceilingHeight)
    {
        printf("Compile Error: Wall with ceiling below floor, will be skipped\n");
        return;
    }

    if (floorHeight == ceilingHeight)
    {
        return;
    }

    std::array<glm::vec3, 4> wallPoints{};
    wallPoints.at(0) = {startPoint.x, ceilingHeight, startPoint.y}; // SC
    wallPoints.at(1) = {endPoint.x, ceilingHeight, endPoint.y}; // EC

    wallPoints.at(2) = {startPoint.x, floorHeight, startPoint.y}; // SF
    wallPoints.at(3) = {endPoint.x, floorHeight, endPoint.y}; // EF

    const glm::vec2 startPointV = {startPoint.x, startPoint.y};
    const glm::vec2 endPointV = {endPoint.x, endPoint.y};
    const float wallLength = glm::distance(startPointV, endPointV);

    for (const glm::vec3 &point: wallPoints)
    {
        ModelVertex v{};
        v.color = lightColor;
        v.normal.x = -wallNormalVector.x;
        v.normal.y = 0;
        v.normal.z = -wallNormalVector.y;

        v.uv.x = previousWallsLength;
        if (point.x == endPoint.x && point.z == endPoint.y)
        {
            v.uv.x += wallLength;
        }
        v.uv.y = -point.y;

        v.uv.x += wallMaterial.uvOffset.x;
        v.uv.y += wallMaterial.uvOffset.y;

        v.uv.x *= wallMaterial.uvScale.x;
        v.uv.y *= wallMaterial.uvScale.y; // TODO is this the correct way to offset+scale?

        v.position = point;
        vertices.push_back(v);
    }

    indices.push_back(2 + currentIndex);
    indices.push_back(1 + currentIndex);
    indices.push_back(0 + currentIndex);

    indices.push_back(1 + currentIndex);
    indices.push_back(2 + currentIndex);
    indices.push_back(3 + currentIndex);

    if (!counterClockWise)
    {
        for (size_t i = indices.size() - 6; i + 2 < indices.size(); i += 3)
        {
            std::swap(indices.at(i), indices.at(i + 2));
        }
    }

    currentIndex += 4;
}


void LevelMeshBuilder::AddSectorBase(const Sector &sector,
                                     const bool isFloor,
                                     const std::vector<const Sector *> &overlapping)
{
    SectorClipper clipper = SectorClipper(sector);
    for (const Sector *s: overlapping)
    {
        clipper.AddHole(s);
    }
    std::vector<glm::vec2> points{};
    std::vector<uint32_t> idx{};
    clipper.ProcessAndMesh(points, idx);

    const WallMaterial &mat = isFloor ? sector.floorMaterial : sector.ceilingMaterial;

    for (const glm::vec2 &point: points)
    {
        ModelVertex v{};
        v.color = sector.lightColor;
        v.normal.x = 0;
        v.normal.z = 0;
        if (isFloor)
        {
            v.normal.y = 1;
        } else
        {
            v.normal.y = -1;
        }
        v.uv = point;
        v.position = {point.x, isFloor ? sector.floorHeight : sector.ceilingHeight, point.y};
        v.uv.x += mat.uvOffset.x;
        v.uv.y += mat.uvOffset.y;

        v.uv.x *= mat.uvScale.x;
        v.uv.y *= mat.uvScale.y; // TODO is this the correct way to offset+scale?
        vertices.push_back(v);
    }

    if (isFloor)
    {
        for (size_t i = 0; i + 2 < idx.size(); i += 3)
        {
            std::swap(idx.at(i), idx.at(i + 2));
        }
    }

    for (const uint32_t i: idx)
    {
        indices.push_back(i + currentIndex);
    }
    currentIndex += points.size();
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

bool LevelMeshBuilder::IsEmpty() const
{
    return vertices.empty() || indices.size() < 3;
}
