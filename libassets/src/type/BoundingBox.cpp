//
// Created by droc101 on 8/20/25.
//

#include <array>
#include <cstdint>
#include <cstdio>
#include <libassets/type/BoundingBox.h>
#include <libassets/type/ModelVertex.h>
#include <libassets/util/DataReader.h>
#include <libassets/util/DataWriter.h>
#include <limits>
#include <vector>

BoundingBox::BoundingBox(DataReader &reader)
{
    for (int i = 0; i < 3; i++)
    {
        origin.at(i) = reader.Read<float>();
    }
    for (int i = 0; i < 3; i++)
    {
        extents.at(i) = reader.Read<float>();
    }
}

BoundingBox::BoundingBox(const std::array<float, 3> extents)
{
    this->extents = extents;
}

BoundingBox::BoundingBox(const std::array<float, 3> origin, const std::array<float, 3> extents)
{
    this->origin = origin;
    this->extents = extents;
}


void BoundingBox::Write(DataWriter &writer) const
{
    writer.WriteBuffer<float, 3>(origin);
    writer.WriteBuffer<float, 3>(extents);
}

BoundingBox::BoundingBox(const std::vector<ModelVertex> &verts)
{
    if (verts.empty())
    {
        printf("WARN: Tried to create AABB with 0 points!");
        return;
    }
    std::array<float, 3> minPoint = {
        std::numeric_limits<float>::max(),
        std::numeric_limits<float>::max(),
        std::numeric_limits<float>::max(),
    };
    std::array<float, 3> maxPoint = {
        std::numeric_limits<float>::lowest(),
        std::numeric_limits<float>::lowest(),
        std::numeric_limits<float>::lowest(),
    };

    for (const ModelVertex &vert: verts)
    {
        for (uint8_t i = 0; i < 3; i++)
        {
            const float &val = vert.position.at(i);
            if (val < minPoint.at(i))
            {
                minPoint.at(i) = val;
            }
            if (val > maxPoint.at(i))
            {
                maxPoint.at(i) = val;
            }
        }
    }

    origin = {
        (minPoint.at(0) + maxPoint.at(0)) * 0.5f,
        (minPoint.at(1) + maxPoint.at(1)) * 0.5f,
        (minPoint.at(2) + maxPoint.at(2)) * 0.5f,
    };
    extents = {
        (maxPoint.at(0) - minPoint.at(0)) * 0.5f,
        (maxPoint.at(1) - minPoint.at(1)) * 0.5f,
        (maxPoint.at(2) - minPoint.at(2)) * 0.5f,
    };
}

BoundingBox::BoundingBox(const std::vector<std::array<float, 3>> &verts)
{
    if (verts.empty())
    {
        printf("WARN: Tried to create AABB with 0 points!");
        return;
    }
    std::array<float, 3> minPoint = {std::numeric_limits<float>::max(),
                                     std::numeric_limits<float>::max(),
                                     std::numeric_limits<float>::max()};
    std::array<float, 3> maxPoint = {std::numeric_limits<float>::lowest(),
                                     std::numeric_limits<float>::lowest(),
                                     std::numeric_limits<float>::lowest()};

    for (const std::array<float, 3> &vert: verts)
    {
        for (uint8_t i = 0; i < 3; i++)
        {
            const float &val = vert.at(i);
            if (val < minPoint.at(i))
            {
                minPoint.at(i) = val;
            }
            if (val > maxPoint.at(i))
            {
                maxPoint.at(i) = val;
            }
        }
    }

    origin = {
        (minPoint.at(0) + maxPoint.at(0)) * 0.5f,
        (minPoint.at(1) + maxPoint.at(1)) * 0.5f,
        (minPoint.at(2) + maxPoint.at(2)) * 0.5f,
    };
    extents = {
        (maxPoint.at(0) - minPoint.at(0)) * 0.5f,
        (maxPoint.at(1) - minPoint.at(1)) * 0.5f,
        (maxPoint.at(2) - minPoint.at(2)) * 0.5f,
    };
}


std::array<std::array<float, 3>, 8> BoundingBox::GetPoints() const
{
    std::array<std::array<float, 3>, 8> points{};

    int i = 0;
    // fun™ for loops
    for (const int dx: {-1, 1})
    {
        for (const int dy: {-1, 1})
        {
            for (const int dz: {-1, 1})
            {
                points.at(i) = {
                    origin.at(0) + static_cast<float>(dx) * extents.at(0),
                    origin.at(1) + static_cast<float>(dy) * extents.at(1),
                    origin.at(2) + static_cast<float>(dz) * extents.at(2),
                };
                i++;
            }
        }
    }

    return points;
}

std::array<float, 24> BoundingBox::GetPointsFlat() const
{
    const std::array<std::array<float, 3>, 8> points = GetPoints();
    std::array<float, 24> flatPoints{};
    for (int i = 0; i < 8; i++)
    {
        for (int j = 0; j < 3; j++)
        {
            const float value = points.at(i).at(j);
            flatPoints.at((i * 3) + j) = value;
        }
    }
    return flatPoints;
}
