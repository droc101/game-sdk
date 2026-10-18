//
// Created by droc101 on 11/18/25.
//

#include "BrushCollisionBuilder.h"
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <glm/vec2.hpp>
#include <libassets/type/Brush.h>
#include <libassets/util/DataWriter.h>
#include <vector>

BrushCollisionBuilder::BrushCollisionBuilder(const Brush &brush)
{
    origin = brush.origin;
    rotation = brush.rotation;
    currentBrush = &brush;
}


void BrushCollisionBuilder::AddFace(const size_t faceIndex)
{
    assert(currentBrush);
    const Brush::Face &face = currentBrush->faces.at(faceIndex);
    const Brush::TriangulatedFace &tris = Brush::TriangulatedFace(*currentBrush, face);
    for (const uint32_t &index: tris.indices)
    {
        const glm::vec3 vertex = tris.vertices.at(index).position;
        if (!std::ranges::contains(vertices, vertex))
        {
            vertices.push_back(vertex);
        }
        const uint32_t actualIndex = std::ranges::find(vertices, vertex) - vertices.begin();
        indices.push_back(actualIndex);
    }
}

void BrushCollisionBuilder::Write(DataWriter &writer)
{
    writer.WriteVec3(origin);
    writer.WriteVec3(glm::radians(rotation));
    writer.Write<size_t>(indices.size() / 3);
    for (size_t i = 0; i < indices.size(); i += 3)
    {
        writer.WriteVec3(vertices.at(indices.at(i)));
        writer.WriteVec3(vertices.at(indices.at(i + 1)));
        writer.WriteVec3(vertices.at(indices.at(i + 2)));
    }
}
