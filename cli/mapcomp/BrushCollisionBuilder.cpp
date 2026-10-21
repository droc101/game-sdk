//
// Created by droc101 on 11/18/25.
//

#include "BrushCollisionBuilder.h"
#include <cassert>
#include <cstddef>
#include <glm/vec2.hpp>
#include <libassets/type/Brush.h>
#include <libassets/util/DataWriter.h>
#include <vector>

BrushCollisionBuilder::BrushCollisionBuilder(const Brush &brush)
{
    origin = brush.origin;
    rotation = brush.rotation;
    for (const glm::vec3 &vertex: brush.vertices)
    {
        vertices.push_back(vertex);
    }
}

void BrushCollisionBuilder::Write(DataWriter &writer) const
{
    writer.WriteVec3(origin);
    writer.WriteVec3(glm::radians(rotation));
    writer.Write<size_t>(vertices.size());
    for (const glm::vec3 &vertex: vertices)
    {
        writer.WriteVec3(vertex);
    }
}
