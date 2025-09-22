//
// Created by droc101 on 9/5/25.
//

#include <algorithm>
#include <array>
#include <cstddef>
#include <libassets/util/Sector.h>
#include <vector>

bool Sector::IsValid()
{
    if (points.size() < 3)
    {
        return false;
    }

    const std::vector<std::array<float, 2>>::iterator it = std::ranges::unique(points).begin();
    if (it != points.end())
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

std::array<float, 4> Sector::CalculateBBox() const
{
    std::array<float, 2> minPoint = {std::numeric_limits<float>::max(), std::numeric_limits<float>::max()};
    std::array<float, 2> maxPoint = {std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest()};

    for (const std::array<float, 2> &vert: points)
    {
        for (uint8_t i = 0; i < 2; i++)
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
    return {minPoint.at(0), minPoint.at(1), maxPoint.at(0), maxPoint.at(1)};
}
