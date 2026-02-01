//
// Created by droc101 on 11/18/25.
//

#include "SectorCollisionBuilder.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <glm/detail/func_geometric.inl>
#include <glm/vec2.hpp>
#include <libassets/type/Sector.h>
#include <mapbox/earcut.hpp>
#include <utility>
#include <vector>
#include "libassets/util/DataWriter.h"

SectorCollisionBuilder::SectorCollisionBuilder(const Sector &sector)
{
    this->sector = &sector;
    glm::vec2 sums{};
    for (const glm::vec2 &point: sector.points)
    {
        sums.x += point.x;
        sums.y += point.y;
    }

    sectorCenter = {};
    sectorCenter.x = sums.x / sector.points.size();
    sectorCenter.z = sums.y / sector.points.size();
    sectorCenter.y = (sector.floorHeight + sector.ceilingHeight) / 2.0f;

    NextShape();
}

void SectorCollisionBuilder::NextShape()
{
    shapes.emplace_back();
}

void SectorCollisionBuilder::AddCeiling()
{
    AddSectorBase(false);
}

void SectorCollisionBuilder::AddFloor()
{
    AddSectorBase(true);
}

void SectorCollisionBuilder::AddSectorBase(const bool isFloor)
{
    std::vector<std::vector<std::array<float, 2>>> polygon{{}};
    for (const glm::vec2 &point: sector->points)
    {
        polygon.at(0).push_back({point.x, point.y});
    }
    std::vector<uint32_t> idx = mapbox::earcut<uint32_t>(polygon);

    for (const glm::vec2 &point: sector->points)
    {
        CurrentShape().vertices.emplace_back(point.x, isFloor ? sector->floorHeight : sector->ceilingHeight, point.y);
    }

    if (isFloor)
    {
        for (size_t i = 0; i + 2 < idx.size(); i += 3)
        {
            std::swap(idx[i], idx[i + 2]);
        }
    }

    for (const uint32_t i: idx)
    {
        CurrentShape().indices.push_back(i + CurrentShape().currentIndex);
    }
    CurrentShape().currentIndex += sector->points.size();
}

void SectorCollisionBuilder::AddWall(const size_t wallIndex)
{
    const glm::vec2 &startPoint = sector->points.at(wallIndex);
    const glm::vec2 &endPoint = sector->points.at((wallIndex + 1) % (sector->points.size()));

    const bool ccw = sector->CalculateArea() > 0;

    AddWallBase(startPoint, endPoint, sector->floorHeight, sector->ceilingHeight, ccw);
}

void SectorCollisionBuilder::AddWallWithGap(const size_t wallIndex, const float adjFloor, const float adjCeil)
{
    if (adjFloor == sector->floorHeight && adjCeil == sector->ceilingHeight)
    {
        return;
    }

    const glm::vec2 &startPoint = sector->points.at(wallIndex);
    const glm::vec2 &endPoint = sector->points.at((wallIndex + 1) % (sector->points.size()));

    const bool ccw = sector->CalculateArea() > 0;

    if (adjFloor > sector->floorHeight)
    {
        AddWallBase(startPoint, endPoint, sector->floorHeight, adjFloor, ccw);
    }
    if (adjCeil < sector->ceilingHeight)
    {
        AddWallBase(startPoint, endPoint, adjCeil, sector->ceilingHeight, ccw);
    }
}


void SectorCollisionBuilder::AddWallBase(const glm::vec2 &startPoint,
                                         const glm::vec2 &endPoint,
                                         const float floorHeight,
                                         const float ceilingHeight,
                                         const bool counterClockWise)
{
    if (floorHeight > ceilingHeight)
    {
        printf("Compile Error: Wall with ceiling below floor, will be skipped");
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

    for (const glm::vec3 &point: wallPoints)
    {
        CurrentShape().vertices.push_back(point);
    }

    CurrentShape().indices.push_back(2 + CurrentShape().currentIndex);
    CurrentShape().indices.push_back(1 + CurrentShape().currentIndex);
    CurrentShape().indices.push_back(0 + CurrentShape().currentIndex);

    CurrentShape().indices.push_back(1 + CurrentShape().currentIndex);
    CurrentShape().indices.push_back(2 + CurrentShape().currentIndex);
    CurrentShape().indices.push_back(3 + CurrentShape().currentIndex);

    if (!counterClockWise)
    {
        for (size_t i = CurrentShape().indices.size() - 6; i + 2 < CurrentShape().indices.size(); i += 3)
        {
            std::swap(CurrentShape().indices[i], CurrentShape().indices[i + 2]);
        }
    }

    CurrentShape().currentIndex += 4;
}

void SectorCollisionBuilder::Write(DataWriter &writer)
{
    std::erase_if(shapes, [](const SubShape &s) { return s.indices.size() < 3 || s.vertices.empty(); });

    writer.WriteVec3(sectorCenter);
    writer.Write<size_t>(shapes.size());
    for (const SubShape &shape: shapes)
    {
        writer.Write<size_t>(shape.indices.size() / 3);
        for (size_t i = 0; i < shape.indices.size(); i += 3)
        {
            WriteIndex(shape.indices.at(i), writer, shape);
            WriteIndex(shape.indices.at(i + 1), writer, shape);
            WriteIndex(shape.indices.at(i + 2), writer, shape);
        }
    }
}

void SectorCollisionBuilder::WriteIndex(const size_t index, DataWriter &writer, const SubShape &shape) const
{
    glm::vec3 vertex = shape.vertices.at(index);
    vertex.x -= sectorCenter.x;
    vertex.y -= sectorCenter.y;
    vertex.z -= sectorCenter.z;
    writer.WriteVec3(vertex);
}

SectorCollisionBuilder::SubShape &SectorCollisionBuilder::CurrentShape()
{
    return shapes.at(shapes.size() - 1);
}
