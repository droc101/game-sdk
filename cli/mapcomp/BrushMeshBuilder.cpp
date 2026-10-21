//
// Created by droc101 on 11/16/25.
//

#include "BrushMeshBuilder.h"
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <glm/glm.hpp>
#include <libassets/type/BoundingBox.h>
#include <libassets/type/Brush.h>
#include <libassets/type/MapVertex.h>
#include <libassets/util/DataWriter.h>
#include <libassets/util/SearchPathManager.h>
#include <string>
#include <vector>
#include "LightBakerGpu.hpp"
#include "LightmapPacker.h"

BrushMeshBuilder::BrushMeshBuilder(const SearchPathManager &pathManager, const std::string &materialPath)
{
    this->pathManager = pathManager;
    this->materialPath = materialPath;
    (void)materialAsset.LoadFromAsset(pathManager.GetAssetPath(materialPath));
}

void BrushMeshBuilder::AddFace(const Brush &brush, const size_t faceIndex)
{
    const Brush::Face &face = brush.faces.at(faceIndex);
    assert(face.material == materialPath);

    Brush::TriangulatedFace triangulatedFace = Brush::TriangulatedFace(brush, face);

    // TODO discard face if it is entirely occluded by faces of other brushes (could be multiple faces on multiple brushes!)

    std::vector<uint32_t> offsetIndices;
    offsetIndices.reserve(triangulatedFace.indices.size());
    for (const uint32_t index: triangulatedFace.indices)
    {
        offsetIndices.emplace_back(index + currentIndex);
    }
    indices.insert(indices.end(), offsetIndices.begin(), offsetIndices.end());

    const glm::mat4 matrix = brush.GetTransformMatrix();
    for (MapVertex &vertex: triangulatedFace.vertices)
    {
        vertex.position = matrix * glm::vec4(vertex.position, 1.0f);
        vertex.normal = matrix * glm::vec4(vertex.normal, 0.0f);
        vertex.emissive = materialAsset.emissive;
        if (!LightBakerGpu::Get().GetTextureIndex(face.material, vertex.textureIndex, pathManager))
        {
            vertex.textureIndex = 0;
        }
    }

    if (!triangulatedFace.vertices.empty())
    {
        faceDatas.push_back(LightmapPacker::CalculateFaceData(face, triangulatedFace, offsetIndices));
    }

    currentIndex += triangulatedFace.vertices.size();
    vertices.insert(vertices.end(), triangulatedFace.vertices.begin(), triangulatedFace.vertices.end());
}

void BrushMeshBuilder::Write(DataWriter &writer) const
{
    writer.WriteString(materialPath);
    const BoundingBox boundingBox{vertices};
    boundingBox.Write(writer);
    writer.Write<uint32_t>(vertices.size());
    for (const MapVertex &vertex: vertices)
    {
        writer.WriteVec3(vertex.position);
        writer.WriteVec2(vertex.uv);
        writer.WriteVec2(vertex.lightmapUv);
        writer.WriteVec3(vertex.normal);
    }
    writer.Write<uint32_t>(indices.size());
    writer.WriteBuffer<uint32_t>(indices);
}

bool BrushMeshBuilder::IsEmpty() const
{
    return vertices.empty() || indices.size() < 3;
}

const std::string &BrushMeshBuilder::GetMaterialPath() const
{
    return materialPath;
}
