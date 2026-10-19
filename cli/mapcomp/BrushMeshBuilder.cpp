//
// Created by droc101 on 11/16/25.
//

#include "BrushMeshBuilder.h"
#include <cassert>
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

// TODO find better place for this
static float MapRange(const float value,
                      const float fromLow,
                      const float fromHigh,
                      const float toLow,
                      const float toHigh)
{
    if (fromLow == fromHigh)
    {
        return toLow;
    }
    const float normalizedValue = (value - fromLow) / (fromHigh - fromLow);
    const float result = toLow + normalizedValue * (toHigh - toLow);
    return result;
}

void BrushMeshBuilder::AddFace(const Brush &brush, const size_t faceIndex)
{
    const Brush::Face &face = brush.faces.at(faceIndex);
    assert(face.material == materialPath);

    Brush::TriangulatedFace triangulatedFace = Brush::TriangulatedFace(brush, face);

    // TODO discard face if it is entirely occluded by faces of other brushes (could be multiple faces on multiple brushes!)

    for (uint32_t &i: triangulatedFace.indices)
    {
        i += currentIndex;
    }
    currentIndex += triangulatedFace.vertices.size();
    indices.insert(indices.end(), triangulatedFace.indices.begin(), triangulatedFace.indices.end());

    const glm::mat4 matrix = brush.GetTransformMatrix();
    for (MapVertex &vertex: triangulatedFace.vertices)
    {
        vertex.position = matrix * glm::vec4(vertex.position, 1.0f);
    }

    if (!triangulatedFace.vertices.empty())
    {
        // const BoundingBox sectorAABB = brush.GetAABB(); // TODO face bounding box
        // const bool rotate = sectorAABB.z < sectorAABB.w;
        // const glm::vec2 sectorTopLeft = {sectorAABB.x - sectorAABB.z, sectorAABB.y - sectorAABB.w};
        // const glm::vec2 sectorBottomRight = {sectorAABB.x + sectorAABB.z, sectorAABB.y + sectorAABB.w};
        // std::vector<glm::vec2> positionsInRect{};
        // for (const uint32_t index: idx)
        // {
        //     const MapVertex &vert = vertices.at(index);
        //     const glm::vec2 position = {vert.position.x, vert.position.z};
        //     float localX = MapRange(position.x, sectorTopLeft.x, sectorBottomRight.x, 0.0f, 1.0f);
        //     float localY = MapRange(position.y, sectorTopLeft.y, sectorBottomRight.y, 0.0f, 1.0f);
        //     if (rotate)
        //     {
        //         std::swap(localX, localY);
        //     }
        //     positionsInRect.emplace_back(localX, localY);
        // }
        //
        // float width = sectorAABB.z * 2;
        // float height = sectorAABB.w * 2;
        // if (rotate)
        // {
        //     std::swap(width, height);
        // }
        // const float unitsPerLuxel = isFloor ? sector.floorMaterial.unitsPerLuxel : sector.ceilingMaterial.unitsPerLuxel;
        // faceIndices.emplace_back(idx, positionsInRect);
        // const float luxelsX = ceilf(fmaxf(width / unitsPerLuxel, 1.0f));
        // const float luxelsY = ceilf(fmaxf(height / unitsPerLuxel, 1.0f));
        // const stbrp_rect rect = {
        //     .id = 0,
        //     .w = static_cast<stbrp_coord>(luxelsX + LightmapHelpers::LIGHTMAP_PADDING * 2),
        //     .h = static_cast<stbrp_coord>(luxelsY + LightmapHelpers::LIGHTMAP_PADDING * 2),
        // };
        // faceRects.push_back(rect);
    }

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
        assert(builder.faceIndices.size() == builder.faceRects.size());
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

        for (size_t i = 0; i < builder.faceIndices.size(); i++)
        {
            const stbrp_rect &rect = rects.at(i + rectIndexBegin);
            const FaceData &faceData = builder.faceIndices.at(i);
            assert(rect.h != 0);
            assert(rect.w != 0);
            for (size_t j = 0; j < faceData.indices.size(); j++)
            {
                const uint32_t index = faceData.indices.at(j);
                MapVertex &vertex = builder.vertices.at(index);
                const glm::vec2 &positionInRect = faceData.positionsInRect.at(j);
                vertex.lightmapUv = LightmapHelpers::GetUv(lightmapSize, rect, positionInRect);
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
