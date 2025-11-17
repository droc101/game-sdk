//
// Created by droc101 on 11/16/25.
//

#include "LevelMeshBuilder.h"
#include <array>
#include <cstddef>
#include <cstdint>
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
    const std::array<float, 2> &startPoint = sector.points.at(wallIndex);
    const std::array<float, 2> &endPoint = sector.points.at((wallIndex + 1) % (sector.points.size() - 1));
    std::array<std::array<float, 3>, 4> wallPoints{};
    wallPoints.at(2) = {startPoint[0], sector.floorHeight, startPoint[1]}; // SF
    wallPoints.at(3) = {endPoint[0], sector.floorHeight, endPoint[1]}; // EF
    wallPoints.at(0) = {startPoint[0], sector.ceilingHeight, startPoint[1]}; // SC
    wallPoints.at(1) = {endPoint[0], sector.ceilingHeight, endPoint[1]}; // EC

    for (const auto &point: wallPoints)
    {
        ModelVertex v{};
        v.color = sector.lightColor;
        // TODO normal
        // TODO uv
        v.position = point;
        vertices.push_back(v);
    }

    indices.push_back(0 + currentIndex);
    indices.push_back(1 + currentIndex);
    indices.push_back(2 + currentIndex);

    indices.push_back(1 + currentIndex);
    indices.push_back(2 + currentIndex);
    indices.push_back(3 + currentIndex);

    currentIndex += 6;
}

void LevelMeshBuilder::AddSectorBase(const Sector &sector, bool isFloor)
{
    std::vector<std::vector<std::array<float, 2>>> polygon{};
    polygon.push_back(sector.points);
    const std::vector<uint32_t> idx = mapbox::earcut<uint32_t>(polygon);

    for (const auto &point: sector.points)
    {
        ModelVertex v{};
        v.color = sector.lightColor;
        // TODO normal
        // TODO uv
        v.position = {point.at(0), isFloor ? sector.floorHeight : sector.ceilingHeight, point.at(1)};
        vertices.push_back(v);
    }
    for (const uint32_t i: idx)
    {
        indices.push_back(i + currentIndex);
    }
    currentIndex += vertices.size();
}

void LevelMeshBuilder::Write(DataWriter &writer) const
{
    writer.Write<uint32_t>(vertices.size());
    for (const ModelVertex &vert: vertices)
    {
        vert.Write(writer);
    }
    writer.Write<uint32_t>(indices.size());
    writer.WriteBuffer<uint32_t>(indices);
}

