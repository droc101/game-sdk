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
    origin = reader.ReadVec3();
    extents = reader.ReadVec3();
}

BoundingBox::BoundingBox(const glm::vec3 extents)
{
    this->extents = extents;
}

BoundingBox::BoundingBox(const glm::vec3 origin, const glm::vec3 extents)
{
    this->origin = origin;
    this->extents = extents;
}


void BoundingBox::Write(DataWriter &writer) const
{
    writer.WriteVec3(origin);
    writer.WriteVec3(extents);
}

BoundingBox::BoundingBox(const std::vector<ModelVertex> &verts)
{
    if (verts.empty())
    {
        printf("WARN: Tried to create AABB with 0 points!");
        return;
    }
    glm::vec3 minPoint = {
        std::numeric_limits<float>::max(),
        std::numeric_limits<float>::max(),
        std::numeric_limits<float>::max(),
    };
    glm::vec3 maxPoint = {
        std::numeric_limits<float>::lowest(),
        std::numeric_limits<float>::lowest(),
        std::numeric_limits<float>::lowest(),
    };

    for (const ModelVertex &vert: verts)
    {
        for (uint8_t i = 0; i < 3; i++)
        {
            if (vert.position.x < minPoint.x)
            {
                minPoint.x = vert.position.x;
            }
            if (vert.position.x > maxPoint.x)
            {
                maxPoint.x = vert.position.x;
            }

            if (vert.position.y < minPoint.y)
            {
                minPoint.y = vert.position.y;
            }
            if (vert.position.y > maxPoint.y)
            {
                maxPoint.y = vert.position.y;
            }

            if (vert.position.z < minPoint.z)
            {
                minPoint.z = vert.position.z;
            }
            if (vert.position.z > maxPoint.z)
            {
                maxPoint.z = vert.position.z;
            }
        }
    }

    origin = {
        (minPoint.x + maxPoint.x) * 0.5f,
        (minPoint.y + maxPoint.y) * 0.5f,
        (minPoint.z + maxPoint.z) * 0.5f,
    };
    extents = {
        (maxPoint.x - minPoint.x) * 0.5f,
        (maxPoint.y - minPoint.y) * 0.5f,
        (maxPoint.z - minPoint.z) * 0.5f,
    };
}

BoundingBox::BoundingBox(const std::vector<glm::vec3> &verts)
{
    if (verts.empty())
    {
        printf("WARN: Tried to create AABB with 0 points!");
        return;
    }
    glm::vec3 minPoint = {std::numeric_limits<float>::max(),
                          std::numeric_limits<float>::max(),
                          std::numeric_limits<float>::max()};
    glm::vec3 maxPoint = {std::numeric_limits<float>::lowest(),
                          std::numeric_limits<float>::lowest(),
                          std::numeric_limits<float>::lowest()};

    for (const glm::vec3 &vert: verts)
    {
        if (vert.x < minPoint.x)
        {
            minPoint.x = vert.x;
        }
        if (vert.x > maxPoint.x)
        {
            maxPoint.x = vert.x;
        }

        if (vert.y < minPoint.y)
        {
            minPoint.y = vert.y;
        }
        if (vert.y > maxPoint.y)
        {
            maxPoint.y = vert.y;
        }

        if (vert.z < minPoint.z)
        {
            minPoint.z = vert.z;
        }
        if (vert.z > maxPoint.z)
        {
            maxPoint.z = vert.z;
        }
    }

    origin = {
        (minPoint.x + maxPoint.x) * 0.5f,
        (minPoint.y + maxPoint.y) * 0.5f,
        (minPoint.z + maxPoint.z) * 0.5f,
    };
    extents = {
        (maxPoint.x - minPoint.x) * 0.5f,
        (maxPoint.y - minPoint.y) * 0.5f,
        (maxPoint.z - minPoint.z) * 0.5f,
    };
}


std::array<glm::vec3, 8> BoundingBox::GetPoints() const
{
    std::array<glm::vec3, 8> points{};

    int i = 0;
    // fun™ for loops
    for (const int dx: {-1, 1})
    {
        for (const int dy: {-1, 1})
        {
            for (const int dz: {-1, 1})
            {
                points.at(i) = {
                    origin.x + static_cast<float>(dx) * extents.x,
                    origin.y + static_cast<float>(dy) * extents.y,
                    origin.z + static_cast<float>(dz) * extents.z,
                };
                i++;
            }
        }
    }

    return points;
}

std::array<float, 24> BoundingBox::GetPointsFlat() const
{
    const std::array<glm::vec3, 8> points = GetPoints();
    std::array<float, 24> flatPoints{};
    for (int i = 0; i < 8; i++)
    {
        flatPoints.at((i * 3) + 0) = points.at(i).x;
        flatPoints.at((i * 3) + 0) = points.at(i).y;
        flatPoints.at((i * 3) + 0) = points.at(i).z;
    }
    return flatPoints;
}
