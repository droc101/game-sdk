//
// Created by droc101 on 8/20/25.
//

#include <array>
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
                points.at(i) = origin + glm::vec3{dx, dy, dz} * extents;
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
        flatPoints.at((i * 3) + 1) = points.at(i).y;
        flatPoints.at((i * 3) + 2) = points.at(i).z;
    }
    return flatPoints;
}
