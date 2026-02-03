//
// Created by droc101 on 9/5/25.
//

#include <algorithm>
#include <array>
#include <cfloat>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <libassets/type/Sector.h>
#include <nlohmann/json.hpp>
#include <vector>

Sector::Sector(nlohmann::ordered_json json)
{
    floorHeight = json.value("floorHeight", -1.0f);
    ceilingHeight = json.value("ceilingHeight", 1.0f);
    floorMaterial = WallMaterial(json["floorMaterial"]);
    ceilingMaterial = WallMaterial(json["ceilingMaterial"]);
    lightColor = Color(json["lightColor"]);
    const nlohmann::ordered_json mats = json.at("wallMaterials");
    for (const nlohmann::basic_json<nlohmann::ordered_map> &material: mats)
    {
        wallMaterials.emplace_back(material);
    }
    const nlohmann::ordered_json jPoints = json.at("points");
    for (const nlohmann::basic_json<nlohmann::ordered_map> &point: jPoints)
    {
        points.emplace_back(point.value("x", 0.0f), point.value("z", 0.0f));
    }
}

bool Sector::ContainsPoint(const glm::vec2 point) const
{
    bool inside = false;
    const size_t n = points.size();
    for (size_t i = 0; i < n; i++)
    {
        const size_t j = (i + n - 1) % n;
        const glm::vec2 &pointI = points.at(i);
        const glm::vec2 &pointJ = points.at(j);

        const bool intersect = ((pointI.y > point.y) != (pointJ.y > point.y)) &&
                               (point.x <
                                (pointJ.x - pointI.x) * (point.y - pointI.y) / (pointJ.y - pointI.y) + pointI.x);

        if (intersect)
        {
            inside = !inside;
        }
    }
    return inside;
}

nlohmann::ordered_json Sector::GenerateJson() const
{
    nlohmann::ordered_json json = nlohmann::ordered_json();
    json["floorHeight"] = floorHeight;
    json["ceilingHeight"] = ceilingHeight;
    json["floorMaterial"] = floorMaterial.GenerateJson();
    json["ceilingMaterial"] = ceilingMaterial.GenerateJson();
    json["lightColor"] = lightColor.GenerateJson();
    json["wallMaterials"] = nlohmann::ordered_json::array();
    for (const WallMaterial &material: wallMaterials)
    {
        json["wallMaterials"].push_back(material.GenerateJson());
    }
    json["points"] = nlohmann::ordered_json::array();
    for (const glm::vec2 &point: points)
    {
        nlohmann::ordered_json jPoint{};
        jPoint["x"] = point.x;
        jPoint["z"] = point.y;
        json["points"].push_back(jPoint);
    }
    return json;
}

double Sector::CalculateArea() const
{
    double area = 0;
    const size_t numPoints = points.size();
    for (size_t i = 0; i < numPoints; i++)
    {
        const size_t j = (i + 1) % numPoints;
        area += points.at(i).x * points.at(j).y - points.at(j).x * points.at(i).y;
    }
    return area * 0.5;
}

glm::vec3 Sector::GetCenter() const
{
    glm::vec3 center{};
    center.y = ceilingHeight + floorHeight / 2.0f;
    for (const glm::vec2 &point: points)
    {
        center.x += point.x;
        center.z += point.y;
    }
    center.x /= static_cast<float>(points.size());
    center.z /= static_cast<float>(points.size());
    return center;
}

glm::vec2 Sector::SegmentNormal(const int segmentIndex) const
{
    const glm::vec2 p0 = points.at(segmentIndex);
    const glm::vec2 p1 = points.at((segmentIndex + 1) % points.size());

    const glm::vec2 edgeDir = p1 - p0;

    const glm::vec2 left = {-edgeDir.y, edgeDir.x};
    const glm::vec2 right = {edgeDir.y, -edgeDir.x};

    const bool ccw = CalculateArea() > 0;

    glm::vec2 nrm = ccw ? right : left;

    const float len = std::sqrt(nrm.x * nrm.x + nrm.y * nrm.y);
    if (len > 1e-12f)
    {
        nrm.x /= len;
        nrm.y /= len;
    }
    return nrm;
}

Sector::SegmentOrientation Sector::GetOrientation(const glm::vec2 &pointA,
                                                  const glm::vec2 &pointB,
                                                  const glm::vec2 &pointC)
{
    const float cross = ((pointB.y - pointA.y) * (pointC.x - pointB.x) - (pointB.x - pointA.x) * (pointC.y - pointB.y));
    if (std::abs(cross) <= FLT_EPSILON)
    {
        return SegmentOrientation::COLINEAR;
    }
    return cross > 0 ? SegmentOrientation::CLOCKWISE : SegmentOrientation::COUNTERCLOCKWISE;
}

bool Sector::PointsEqual(const glm::vec2 &a, const glm::vec2 &b)
{
    return std::abs(a.x - b.x) <= FLT_EPSILON && std::abs(a.y - b.y) <= FLT_EPSILON;
}

bool Sector::PointOnSegment(const glm::vec2 &a, const glm::vec2 &b, const glm::vec2 &c)
{
    return std::abs(c.x - std::max(a.x, b.x)) <= FLT_EPSILON && std::abs(c.y - std::max(a.y, b.y)) <= FLT_EPSILON;
}

bool Sector::CheckIntersection(const glm::vec2 &segmentAStart,
                               const glm::vec2 &segmentAEnd,
                               const glm::vec2 &segmentBStart,
                               const glm::vec2 &segmentBEnd)
{
    if (PointsEqual(segmentAStart, segmentBStart) ||
        PointsEqual(segmentAStart, segmentBEnd) ||
        PointsEqual(segmentAEnd, segmentBStart) ||
        PointsEqual(segmentAEnd, segmentBEnd))
    {
        return false;
    }


    const SegmentOrientation orientationABC = GetOrientation(segmentAStart, segmentAEnd, segmentBStart);
    const SegmentOrientation orientationABD = GetOrientation(segmentAStart, segmentAEnd, segmentBEnd);
    const SegmentOrientation orientationCDA = GetOrientation(segmentBStart, segmentBEnd, segmentAStart);
    const SegmentOrientation orientationCDB = GetOrientation(segmentBStart, segmentBEnd, segmentAEnd);

    if (orientationABC != orientationABD && orientationCDA != orientationCDB)
    {
        return true;
    }

    if (orientationABC == SegmentOrientation::COLINEAR && PointOnSegment(segmentAStart, segmentAEnd, segmentBStart))
    {
        return true;
    }
    if (orientationABD == SegmentOrientation::COLINEAR && PointOnSegment(segmentAStart, segmentAEnd, segmentBEnd))
    {
        return true;
    }
    if (orientationCDA == SegmentOrientation::COLINEAR && PointOnSegment(segmentBStart, segmentBEnd, segmentAStart))
    {
        return true;
    }
    if (orientationCDB == SegmentOrientation::COLINEAR && PointOnSegment(segmentBStart, segmentBEnd, segmentAEnd))
    {
        return true;
    }

    return false;
}

bool Sector::IsValid() const
{
    const size_t n = points.size();
    for (size_t i = 0; i < n; i++)
    {
        const size_t iNext = (i + n - 1) % n;
        const glm::vec2 &aStart = points.at(i);
        const glm::vec2 &aEnd = points.at(iNext);

        for (size_t j = 0; j < n; j++)
        {
            if (i == j)
            {
                continue;
            }
            const size_t jNext = (j + n - 1) % n;
            const glm::vec2 &bStart = points.at(j);
            const glm::vec2 &bEnd = points.at(jNext);

            if (PointsEqual(aStart, bStart) && PointsEqual(aEnd, bEnd))
            {
                continue; // exact overlaps allowed
            }

            if (CheckIntersection(aStart, aEnd, bStart, bEnd))
            {
                return false;
            }
        }
    }

    return true;
}
