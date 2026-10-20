//
// Created by droc101 on 11/16/25.
//

#include "BrushMeshBuilder.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <glm/glm.hpp>
#include <libassets/asset/LevelMaterialAsset.h>
#include <libassets/type/BoundingBox.h>
#include <libassets/type/Brush.h>
#include <libassets/type/MapVertex.h>
#include <libassets/type/Material.h>
#include <libassets/util/DataWriter.h>
#include <libassets/util/LightmapHelpers.hpp>
#include <libassets/util/Logger.h>
#include <libassets/util/SearchPathManager.h>
#include <stb_rect_pack.h>
#include <string>
#include <vector>

BrushMeshBuilder::BrushMeshBuilder(const SearchPathManager &pathManager, const std::string &materialPath)
{
    this->pathManager = pathManager;
    this->materialPath = materialPath;
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
    }

    if (!triangulatedFace.vertices.empty())
    {
        std::vector<glm::vec2> positionsInRect;
        positionsInRect.reserve(triangulatedFace.indices.size());
        for (const uint32_t index: triangulatedFace.indices)
        {
            positionsInRect.emplace_back(triangulatedFace.vertices.at(index).lightmapUv);
        }

        const float width = std::max(triangulatedFace.faceSize.x, triangulatedFace.faceSize.y);
        const float height = std::min(triangulatedFace.faceSize.x, triangulatedFace.faceSize.y);
        faceDatas.emplace_back(offsetIndices, positionsInRect);
        const float luxelsX = std::ceil(std::max(width / face.unitsPerLuxel, 1.0f));
        const float luxelsY = std::ceil(std::max(height / face.unitsPerLuxel, 1.0f));
        faceRects.emplace_back(0,
                               static_cast<stbrp_coord>(luxelsX + LightmapHelpers::LIGHTMAP_PADDING * 2),
                               static_cast<stbrp_coord>(luxelsY + LightmapHelpers::LIGHTMAP_PADDING * 2));
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

bool BrushMeshBuilder::CalculateLightmapUvs(glm::uvec2 &lightmapSize,
                                            std::vector<BrushMeshBuilder> &meshBuilders,
                                            const SearchPathManager &pathMgr)
{
    for (const BrushMeshBuilder &builder: meshBuilders)
    {
        assert(builder.faceDatas.size() == builder.faceRects.size());
    }

    std::vector<stbrp_rect> rects{};
    for (const BrushMeshBuilder &builder: meshBuilders)
    {
        const std::string materialPath = pathMgr.GetAssetPath(builder.GetMaterialPath());
        LevelMaterialAsset material{};
        material.LoadFromAsset(materialPath);
        if (material.shader == Material::MaterialShader::SHADER_SHADED)
        {
            rects.insert(rects.end(), builder.faceRects.begin(), builder.faceRects.end());
        }
    }

    if (!LightmapHelpers::FitLightmap(rects, lightmapSize))
    {
        return false;
    }
    Logger::Info("Lightmap size: {} by {}", lightmapSize.x, lightmapSize.y);

    size_t rectIndexBegin = 0;
    for (BrushMeshBuilder &builder: meshBuilders)
    {
        const std::string materialPath = pathMgr.GetAssetPath(builder.GetMaterialPath());
        LevelMaterialAsset material{};
        material.LoadFromAsset(materialPath);
        if (material.shader != Material::MaterialShader::SHADER_SHADED)
        {
            continue;
        }

        for (size_t i = 0; i < builder.faceDatas.size(); i++)
        {
            const stbrp_rect &rect = rects.at(i + rectIndexBegin);
            const FaceData &faceData = builder.faceDatas.at(i);
            assert(rect.h != 0);
            assert(rect.w != 0);
            for (size_t j = 0; j < faceData.indices.size(); j++)
            {
                const uint32_t index = faceData.indices.at(j);
                MapVertex &vertex = builder.vertices.at(index);
                const glm::vec2 &positionInRect = faceData.positionsInRect.at(j);
                vertex.lightmapUv = LightmapHelpers::GetUv(lightmapSize, rect, positionInRect);
                if (vertex.lightmapUv.x < 0 || vertex.lightmapUv.y < 0)
                {
                    asm("int3");
                }
                asm("nop");
            }
        }
        rectIndexBegin += builder.faceRects.size();
    }

    return true;
}

const std::string &BrushMeshBuilder::GetMaterialPath() const
{
    return materialPath;
}
