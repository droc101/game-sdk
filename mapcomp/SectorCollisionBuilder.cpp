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
}


void SectorCollisionBuilder::AddCeiling()
{
    AddSectorBase(false);
}

void SectorCollisionBuilder::AddFloor()
{
    AddSectorBase(true);
}

void SectorCollisionBuilder::AddSectorBase(bool isFloor)
{
    const std::vector<std::vector<std::array<float, 2>>> polygon{sector->points};
    std::vector<uint32_t> idx = mapbox::earcut<uint32_t>(polygon);

    for (const std::array<float, 2> &point: sector->points)
    {
        vertices.push_back({point.at(0), isFloor ? sector->floorHeight : sector->ceilingHeight, point.at(1)});
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
        indices.push_back(i + currentIndex);
    }
    currentIndex += sector->points.size();
}

void SectorCollisionBuilder::AddWall(size_t wallIndex)
{
    const std::array<float, 2> &startPoint = sector->points.at(wallIndex);
    const std::array<float, 2> &endPoint = sector->points.at((wallIndex + 1) % (sector->points.size()));

    const bool ccw = sector->CalculateArea() > 0;

    AddWallBase(startPoint, endPoint, sector->floorHeight, sector->ceilingHeight, ccw);
}

void SectorCollisionBuilder::AddWallWithGap(size_t wallIndex, float adjFloor, float adjCeil)
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
                                         float floorHeight,
                                         float ceilingHeight,
                                         bool counterClockWise)
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
        vertices.push_back(point);
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
            std::swap(indices[i], indices[i + 2]);
        }
    }

    currentIndex += 4;
}

void SectorCollisionBuilder::Write(DataWriter &writer) const
{
    writer.Write<size_t>(indices.size() / 3);
    for (size_t i = 0; i < indices.size(); i += 3)
    {
        writer.WriteBuffer<float>(vertices.at(indices.at(i)));
        writer.WriteBuffer<float>(vertices.at(indices.at(i + 1)));
        writer.WriteBuffer<float>(vertices.at(indices.at(i + 2)));
    }
}
