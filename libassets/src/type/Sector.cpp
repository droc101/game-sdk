//
// Created by droc101 on 9/5/25.
//

#include <algorithm>
#include <array>
#include <cfloat>
#include <cstddef>
#include <cstdlib>
#include <libassets/type/Sector.h>
#include <nlohmann/json.hpp>
#include <vector>

Sector::Sector(nlohmann::ordered_json j)
{
    floorHeight = j.value("floorHeight", -1.0f);
    ceilingHeight = j.value("ceilingHeight", 1.0f);
    floorMaterial = WallMaterial(j["floorMaterial"]);
    ceilingMaterial = WallMaterial(j["ceilingMaterial"]);
    lightColor = Color(j["lightColor"]);
    const nlohmann::ordered_json mats = j.at("wallMaterials");
    for (const nlohmann::basic_json<nlohmann::ordered_map> &material: mats)
    {
        wallMaterials.emplace_back(WallMaterial(material));
    }
    const nlohmann::ordered_json jPoints = j.at("points");
    for (const nlohmann::basic_json<nlohmann::ordered_map> &point: jPoints)
    {
        points.push_back({point.value("x", 0.0f), point.value("z", 0.0f)});
    }
}

bool Sector::ContainsPoint(const std::array<float, 2> point) const
{
    bool inside = false;
    const size_t n = points.size();
    for (size_t i = 0; i < n; i++)
    {
        const size_t j = (i + n - 1) % n;
        const std::array<float, 2> &pi = points.at(i);
        const std::array<float, 2> &pj = points.at(j);

        const bool intersect = ((pi.at(1) > point.at(1)) != (pj.at(1) > point.at(1))) &&
                               (point.at(0) <
                                (pj.at(0) - pi.at(0)) * (point.at(1) - pi.at(1)) / (pj.at(1) - pi.at(1)) + pi.at(0));

        if (intersect)
        {
            inside = !inside;
        }
    }
    return inside;
}

nlohmann::ordered_json Sector::GenerateJson() const
{
    nlohmann::ordered_json j = nlohmann::ordered_json();
    j["floorHeight"] = floorHeight;
    j["ceilingHeight"] = ceilingHeight;
    j["floorMaterial"] = floorMaterial.GenerateJson();
    j["ceilingMaterial"] = ceilingMaterial.GenerateJson();
    j["lightColor"] = lightColor.GenerateJson();
    j["wallMaterials"] = nlohmann::ordered_json::array();
    for (const WallMaterial &material: wallMaterials)
    {
        j["wallMaterials"].push_back(material.GenerateJson());
    }
    j["points"] = nlohmann::ordered_json::array();
    for (const std::array<float, 2> &point: points)
    {
        nlohmann::ordered_json jPoint{};
        jPoint["x"] = point.at(0);
        jPoint["z"] = point.at(1);
        j["points"].push_back(jPoint);
    }
    return j;
}

double Sector::CalculateArea() const
{
    double area = 0;
    const size_t numPoints = points.size();
    for (size_t i = 0; i < numPoints; i++)
    {
        const size_t j = (i + 1) % numPoints;
        area += points.at(i).at(0) * points.at(j).at(1) - points.at(j).at(0) * points.at(i).at(1);
    }
    return area * 0.5;
}

std::array<float, 3> Sector::GetCenter() const
{
    std::array<float, 3> center{};
    center.at(1) = ceilingHeight + floorHeight / 2.0f;
    for (const std::array<float, 2> &point: points)
    {
        center.at(0) += point.at(0);
        center.at(2) += point.at(1);
    }
    center.at(0) /= static_cast<float>(points.size());
    center.at(2) /= static_cast<float>(points.size());
    return center;
}

std::array<float, 2> Sector::SegmentNormal(const int segmentIndex) const
{
    const std::array<float, 2> p0 = points.at(segmentIndex);
    const std::array<float, 2> p1 = points.at((segmentIndex + 1) % points.size());

    const std::array<float, 2> edgeDir = {p1.at(0) - p0.at(0), p1.at(1) - p0.at(1)};

    const std::array<float, 2> left = {-edgeDir.at(1), edgeDir.at(0)};
    const std::array<float, 2> right = {edgeDir.at(1), -edgeDir.at(0)};

    const bool ccw = CalculateArea() > 0;

    std::array<float, 2> nrm = ccw ? right : left;

    const float len = std::sqrt(nrm.at(0) * nrm.at(0) + nrm.at(1) * nrm.at(1));
    if (len > 1e-12f)
    {
        nrm.at(0) /= len;
        nrm.at(1) /= len;
    }
    return nrm;
}

Sector::SegmentOrientation Sector::GetOrientation(const std::array<float, 2> &pointA,
                                                  const std::array<float, 2> &pointB,
                                                  const std::array<float, 2> &pointC)
{
    const float cross = ((pointB.at(1) - pointA.at(1)) * (pointC.at(0) - pointB.at(0)) -
                         (pointB.at(0) - pointA.at(0)) * (pointC.at(1) - pointB.at(1)));
    if (std::abs(cross) <= FLT_EPSILON)
    {
        return SegmentOrientation::COLINEAR;
    }
    return cross > 0 ? SegmentOrientation::CLOCKWISE : SegmentOrientation::COUNTERCLOCKWISE;
}

bool Sector::PointsEqual(const std::array<float, 2> &a, const std::array<float, 2> &b)
{
    return std::abs(a.at(0) - b.at(0)) <= FLT_EPSILON && std::abs(a.at(1) - b.at(1)) <= FLT_EPSILON;
}

bool Sector::PointOnSegment(const std::array<float, 2> &a, const std::array<float, 2> &b, const std::array<float, 2> &c)
{
    return c.at(0) <= std::max(a.at(0), b.at(0)) + FLT_EPSILON &&
           c.at(0) >= std::min(a.at(0), b.at(0)) - FLT_EPSILON &&
           c.at(1) <= std::max(a.at(1), b.at(1)) + FLT_EPSILON &&
           c.at(1) >= std::min(a.at(1), b.at(1)) - FLT_EPSILON;
}

bool Sector::CheckIntersection(const std::array<float, 2> &segmentAStart,
                               const std::array<float, 2> &segmentAEnd,
                               const std::array<float, 2> &segmentBStart,
                               const std::array<float, 2> &segmentBEnd)
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
        const std::array<float, 2> &aStart = points.at(i);
        const std::array<float, 2> &aEnd = points.at(iNext);

        for (size_t j = 0; j < n; j++)
        {
            if (i == j)
            {
                continue;
            }
            const size_t jNext = (j + n - 1) % n;
            const std::array<float, 2> &bStart = points.at(j);
            const std::array<float, 2> &bEnd = points.at(jNext);

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
