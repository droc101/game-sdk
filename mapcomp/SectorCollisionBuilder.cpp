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
    std::array<float, 2> sums{};
    for (const std::array<float, 2> &point: sector.points)
    {
        sums[0] += point[0];
        sums[1] += point[1];
    }

    sectorCenter[0] = sums[0] / sector.points.size();
    sectorCenter[2] = sums[1] / sector.points.size();
    sectorCenter[1] = (sector.floorHeight + sector.ceilingHeight) / 2.0f;

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
    const std::vector<std::vector<std::array<float, 2>>> polygon{sector->points};
    std::vector<uint32_t> idx = mapbox::earcut<uint32_t>(polygon);

    for (const std::array<float, 2> &point: sector->points)
    {
        CurrentShape().vertices.push_back({point.at(0), isFloor ? sector->floorHeight : sector->ceilingHeight, point.at(1)});
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
    const std::array<float, 2> &startPoint = sector->points.at(wallIndex);
    const std::array<float, 2> &endPoint = sector->points.at((wallIndex + 1) % (sector->points.size()));

    const bool ccw = sector->CalculateArea() > 0;

    AddWallBase(startPoint, endPoint, sector->floorHeight, sector->ceilingHeight, ccw);
}

void SectorCollisionBuilder::AddWallWithGap(const size_t wallIndex, const float adjFloor, const float adjCeil)
{
    if (adjFloor == sector->floorHeight && adjCeil == sector->ceilingHeight)
    {
        return;
    }

    const std::array<float, 2> &startPoint = sector->points.at(wallIndex);
    const std::array<float, 2> &endPoint = sector->points.at((wallIndex + 1) % (sector->points.size()));

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


void SectorCollisionBuilder::AddWallBase(const std::array<float, 2> &startPoint,
                                         const std::array<float, 2> &endPoint,
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

    std::array<std::array<float, 3>, 4> wallPoints{};
    wallPoints.at(0) = {startPoint[0], ceilingHeight, startPoint[1]}; // SC
    wallPoints.at(1) = {endPoint[0], ceilingHeight, endPoint[1]}; // EC

    wallPoints.at(2) = {startPoint[0], floorHeight, startPoint[1]}; // SF
    wallPoints.at(3) = {endPoint[0], floorHeight, endPoint[1]}; // EF

    for (const std::array<float, 3> &point: wallPoints)
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

void SectorCollisionBuilder::Write(DataWriter &writer) const
{
    writer.WriteBuffer<float>(sectorCenter);
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
    std::array<float, 3> vertex = shape.vertices.at(index);
    vertex.at(0) -= sectorCenter.at(0);
    vertex.at(1) -= sectorCenter.at(1);
    vertex.at(2) -= sectorCenter.at(2);
    writer.WriteBuffer<float>(vertex);
}

SectorCollisionBuilder::SubShape &SectorCollisionBuilder::CurrentShape()
{
    return shapes.at(shapes.size() - 1);
}

