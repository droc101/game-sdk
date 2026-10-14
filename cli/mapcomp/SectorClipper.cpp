//
// Created by droc101 on 2/3/26.
//

#include "SectorClipper.h"
#include <array>
#include <clipper2/clipper.core.h>
#include <clipper2/clipper.h>
#include <cstddef>
#include <cstdint>
#include <libassets/type/Sector.h>
#include <mapbox/earcut.hpp>
#include <numeric>
#include <vector>

SectorClipper::SectorClipper(const Sector &sector)
{
    for (const glm::vec2 &point: sector.points)
    {
        polygon.emplace_back(point.x, point.y);
    }
}

void SectorClipper::AddHole(const Sector *sector)
{
    Clipper2Lib::PathD holePolygon{};
    for (const glm::vec2 &point: sector->points)
    {
        holePolygon.emplace_back(point.x, point.y);
    }

    holes.push_back(holePolygon);
}

void SectorClipper::ProcessAndMesh(std::vector<glm::vec2> &vertices, std::vector<uint32_t> &indices) const
{
    const Clipper2Lib::PathsD subjects = {polygon};

    const Clipper2Lib::PathsD result = Clipper2Lib::Difference(subjects, holes, Clipper2Lib::FillRule::NonZero);

    if (result.empty())
    {
        return;
    }

    std::vector<Clipper2Lib::PathD> outers;
    std::vector<Clipper2Lib::PathD> holesOut;

    for (const Clipper2Lib::PathD &path: result)
    {
        if (Area(path) > 0)
        {
            outers.push_back(path);
        } else
        {
            holesOut.push_back(path);
        }
    }

    indices.clear();
    vertices.clear();

    uint32_t indexOffset = 0;

    for (const Clipper2Lib::PathD &outer: outers)
    {
        std::vector<std::vector<std::array<float, 2>>> rings = {{}};
        for (const Clipper2Lib::PointD &point: outer)
        {
            rings.at(0).push_back({static_cast<float>(point.x), static_cast<float>(point.y)});
        }

        for (const Clipper2Lib::PathD &hole: holesOut)
        {
            if (PointInPolygon(hole.at(0), outer) != Clipper2Lib::PointInPolygonResult::IsOutside)
            {
                rings.emplace_back();
                for (const Clipper2Lib::PointD &pt: hole)
                {
                    rings.back().push_back({static_cast<float>(pt.x), static_cast<float>(pt.y)});
                }
            }
        }

        const std::vector<uint32_t> localIndices = mapbox::earcut<uint32_t>(rings);

        for (const std::vector<std::array<float, 2>> &ring: rings)
        {
            for (const std::array<float, 2> &v: ring)
            {
                vertices.emplace_back(v.at(0), v.at(1));
            }
        }

        for (const uint32_t i: localIndices)
        {
            indices.push_back(i + indexOffset);
        }

        // todo do this without the stupid inline function thingy
        indexOffset += static_cast<uint32_t>(std::accumulate(rings.begin(),
                                                             rings.end(),
                                                             0,
                                                             [](const size_t s,
                                                                const std::vector<std::array<float, 2>> &r) {
                                                                 return s + r.size();
                                                             }));
    }
}
