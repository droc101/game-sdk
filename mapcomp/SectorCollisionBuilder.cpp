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
#include <libassets/util/DataWriter.h>
#include <utility>
#include <vector>
#include "SectorClipper.h"

SectorCollisionBuilder::SectorCollisionBuilder(const Sector &sector)
{
    this->sector = &sector;
    glm::vec2 sums{};
    for (const glm::vec2 &point: sector.points)
    {
        sums += point;
    }

    sectorCenter = {};
    sectorCenter.x = sums.x / static_cast<float>(sector.points.size());
    sectorCenter.y = (sector.floorHeight + sector.ceilingHeight) / 2.0f;
    sectorCenter.z = sums.y / static_cast<float>(sector.points.size());

    NextShape();
}

void SectorCollisionBuilder::NextShape()
{
    shapes.emplace_back();
}

void SectorCollisionBuilder::AddCeiling(const std::vector<const Sector *> &overlapping)
{
    AddSectorBase(false, overlapping);
}

void SectorCollisionBuilder::AddFloor(const std::vector<const Sector *> &overlapping)
{
    AddSectorBase(true, overlapping);
}

void SectorCollisionBuilder::AddSectorBase(const bool isFloor, const std::vector<const Sector *> &overlapping)
{
    SectorClipper clipper = SectorClipper(*sector);
    for (const Sector *s: overlapping)
    {
        clipper.AddHole(s);
    }
    std::vector<glm::vec2> points{};
    std::vector<uint32_t> idx{};
    clipper.ProcessAndMesh(points, idx);

    for (const glm::vec2 &point: points)
    {
        CurrentShape().vertices.emplace_back(point.x, isFloor ? sector->floorHeight : sector->ceilingHeight, point.y);
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
        CurrentShape().indices.push_back(i + CurrentShape().currentIndex);
    }
    CurrentShape().currentIndex += points.size();
}

void SectorCollisionBuilder::AddWall(const size_t wallIndex, const float floorHeight, const float ceilingHeight)
{
    const glm::vec2 &startPoint = sector->points.at(wallIndex);
    const glm::vec2 &endPoint = sector->points.at((wallIndex + 1) % (sector->points.size()));

    const bool ccw = sector->CalculateArea() > 0;

    AddWallBase(startPoint, endPoint, floorHeight, ceilingHeight, ccw);
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
            std::swap(CurrentShape().indices.at(i), CurrentShape().indices.at(i + 2));
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
    writer.WriteVec3(shape.vertices.at(index) - sectorCenter);
}

SectorCollisionBuilder::SubShape &SectorCollisionBuilder::CurrentShape()
{
    return shapes.at(shapes.size() - 1);
}
