//
// Created by droc101 on 9/6/25.
//

#include "LevelEditor.h"
#include <cmath>

float LevelEditor::SnapToGrid(const float f)
{
    if (!snapToGrid)
    {
        return f;
    }
    const float gridSize = GRID_SPACING_VALUES.at(gridSpacingIndex);
    const float nf = f / gridSize;
    const float fs = std::round(nf);
    return fs * gridSize;
}

glm::vec3 LevelEditor::SnapToGrid(const glm::vec3 v)
{
    return {SnapToGrid(v.x), SnapToGrid(v.y), SnapToGrid(v.z)};
}

bool LevelEditor::IsPointInBounds(glm::vec3 p)
{
    const bool x = p.x >= -512 && p.x <= 512;
    const bool y = p.y >= -512 && p.y <= 512;
    const bool z = p.z >= -512 && p.z <= 512;
    return x && y && z;
}

float LevelEditor::VecDistanceToLine2D(const glm::vec2 lineStart, const glm::vec2 lineEnd, const glm::vec2 testPoint)
{
    const float lineMag = distance(lineStart, lineEnd);

    if (lineMag == 0.0)
    {
        return distance(lineStart, testPoint);
    }

    const float u = (((testPoint.x - lineStart.x) * (lineEnd.x - lineStart.x)) +
                     ((testPoint.y - lineStart.y) * (lineEnd.y - lineStart.y))) /
                    (lineMag * lineMag);

    if (u < 0.0)
    {
        return distance(testPoint, lineStart);
    }
    if (u > 1.0)
    {
        return distance(testPoint, lineEnd);
    }

    glm::vec2 intersection;
    intersection.x = lineStart.x + u * (lineEnd.x - lineStart.x);
    intersection.y = lineStart.y + u * (lineEnd.y - lineStart.y);

    return distance(testPoint, intersection);
}
