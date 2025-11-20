//
// Created by droc101 on 9/5/25.
//

#include <algorithm>
#include <array>
#include <cstddef>
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


bool Sector::IsValid() const
{
    if (points.size() < 3)
    {
        return false;
    }

    std::vector<std::array<float, 2>> points_copy = points;
    const std::vector<std::array<float, 2>>::iterator it = std::ranges::unique(points_copy).begin();
    if (it != points_copy.end())
    {
        return false;
    }

    for (size_t i = 0; i < points.size(); i++)
    {
        const std::array<float, 2> &edge1_start = points[i];
        const std::array<float, 2> &edge1_end = points[(i + 1) % points.size()];
        for (size_t j = i + 1; j < points.size(); j++)
        {
            const std::array<float, 2> &edge2_start = points[j];
            const std::array<float, 2> &edge2_end = points[(j + 1) % points.size()];

            if (j == i || (j + 1) % points.size() == i || (i + 1) % points.size() == j)
            {
                continue;
            }

            if (CheckIntersection(edge1_start, edge1_end, edge2_start, edge2_end))
            {
                return false;
            }
        }
    }

    return true;
}

Sector::SegmentOrientation Sector::GetOrientation(const std::array<float, 2> &pointA,
                                                  const std::array<float, 2> &pointB,
                                                  const std::array<float, 2> &pointC)
{
    const float cross = ((pointB[1] - pointA[1]) * (pointC[0] - pointB[0]) -
                         (pointB[0] - pointA[0]) * (pointC[1] - pointB[1]));
    if (cross == 0)
    {
        return SegmentOrientation::COLINEAR;
    }
    return cross > 0 ? SegmentOrientation::CLOCKWISE : SegmentOrientation::COUNTERCLOCKWISE;
}

bool Sector::OnSegment(const std::array<float, 2> &segment_start,
                       const std::array<float, 2> &point,
                       const std::array<float, 2> &segment_end)
{
    const bool xBoundsCheckA = std::min(segment_start[0], segment_end[0]) <= point[0];
    const bool xBoundsCheckB = point[0] <= std::max(segment_start[0], segment_end[0]);
    const bool yBoundsCheckA = std::min(segment_start[1], segment_end[1]) <= point[1];
    const bool yBoundsCheckB = point[0] <= std::max(segment_start[1], segment_end[1]);
    return xBoundsCheckA && xBoundsCheckB && yBoundsCheckA && yBoundsCheckB;
}

bool Sector::CheckIntersection(const std::array<float, 2> &segmentAStart,
                               const std::array<float, 2> &segmentAEnd,
                               const std::array<float, 2> &segmentBStart,
                               const std::array<float, 2> &segmentBEnd)
{
    const SegmentOrientation orientationA = GetOrientation(segmentAStart, segmentAEnd, segmentBStart);
    const SegmentOrientation orientationB = GetOrientation(segmentAStart, segmentAEnd, segmentBEnd);
    const SegmentOrientation orientationC = GetOrientation(segmentBStart, segmentBEnd, segmentAStart);
    const SegmentOrientation orientationD = GetOrientation(segmentBStart, segmentBEnd, segmentAEnd);

    if (orientationA != orientationB && orientationC != orientationD)
    {
        return true;
    }

    if (orientationA == SegmentOrientation::COLINEAR && OnSegment(segmentAStart, segmentBStart, segmentAEnd))
    {
        return true;
    }
    if (orientationB == SegmentOrientation::COLINEAR && OnSegment(segmentAStart, segmentBEnd, segmentAEnd))
    {
        return true;
    }
    if (orientationC == SegmentOrientation::COLINEAR && OnSegment(segmentBStart, segmentAStart, segmentBEnd))
    {
        return true;
    }
    if (orientationD == SegmentOrientation::COLINEAR && OnSegment(segmentBStart, segmentBEnd, segmentAEnd))
    {
        return true;
    }
    return false;
}

bool Sector::ContainsPoint(const std::array<float, 2> point) const
{
    bool inside = false;
    const size_t n = points.size();
    for (size_t i = 0; i < n; i++)
    {
        const size_t j = (i + n - 1) % n;
        const std::array<float, 2> &pi = points[i];
        const std::array<float, 2> &pj = points[j];

        const bool intersect = ((pi[1] > point[1]) != (pj[1] > point[1])) &&
                               (point[0] < (pj[0] - pi[0]) * (point[1] - pi[1]) / (pj[1] - pi[1]) + pi[0]);

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
    const int numPoints = points.size();
    for (int i = 0; i < numPoints; i++)
    {
        int j = (i + 1) % numPoints;
        area += points[i][0] * points[j][1] - points[j][0] * points[i][1];
    }
    return area * 0.5;
}

std::array<float, 2> Sector::SegmentNormal(const int segmentIndex) const
{
    const std::array<float, 2> p0 = points[segmentIndex];
    const std::array<float, 2> p1 = points[(segmentIndex + 1) % points.size()];

    const std::array<float, 2> edgeDir = {p1[0] - p0[0], p1[1] - p0[1]};

    const std::array<float, 2> left = {-edgeDir[1], edgeDir[0]};
    const std::array<float, 2> right = {edgeDir[1], -edgeDir[0]};

    const bool ccw = CalculateArea() > 0;

    std::array<float, 2> nrm = ccw ? right : left;

    const float len = std::sqrt(nrm[0] * nrm[0] + nrm[1] * nrm[1]);
    if (len > 1e-12f)
    {
        nrm[0] /= len;
        nrm[1] /= len;
    }
    return nrm;
}
