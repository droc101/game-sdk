//
// Created by droc101 on 10/20/26.
//

#include "LightmapPacker.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <libassets/asset/LevelMaterialAsset.h>
#include <libassets/type/MapVertex.h>
#include <libassets/type/Material.h>
#include <libassets/util/LightmapHelpers.hpp>
#include <libassets/util/Logger.h>
#include <libassets/util/SearchPathManager.h>
#include <string>
#include <vector>
#include "BrushMeshBuilder.h"

bool LightmapPacker::CalculateLightmapUvs(glm::uvec2 &lightmapSize,
                                          std::vector<BrushMeshBuilder> &meshBuilders,
                                          const SearchPathManager &pathMgr)
{
    std::vector<stbrp_rect> rects{};
    for (const BrushMeshBuilder &builder: meshBuilders)
    {
        const std::string materialPath = pathMgr.GetAssetPath(builder.GetMaterialPath());
        LevelMaterialAsset material{};
        material.LoadFromAsset(materialPath);
        if (material.shader == Material::MaterialShader::SHADER_SHADED)
        {
            for (const FaceData &faceData: builder.faceDatas)
            {
                rects.push_back(faceData.rect);
            }
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
            for (size_t _ = 0; _ < builder.faceDatas.size(); _++)
            {
                for (MapVertex &vertex: builder.vertices)
                {
                    vertex.lightmapUv = glm::vec2{0};
                }
            }
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
            }
        }
        rectIndexBegin += builder.faceDatas.size();
    }

    return true;
}

LightmapPacker::FaceData LightmapPacker::CalculateFaceData(const Brush::Face &face,
                                                           const Brush::TriangulatedFace &triangulatedFace,
                                                           const std::vector<uint32_t> &offsetIndices)
{
    std::vector<glm::vec2> positionsInRect;
    positionsInRect.reserve(triangulatedFace.indices.size());
    for (const uint32_t index: triangulatedFace.indices)
    {
        positionsInRect.emplace_back(triangulatedFace.vertices.at(index).lightmapUv);
    }

    const float width = triangulatedFace.faceSize.x;
    const float height = triangulatedFace.faceSize.y;
    const float luxelsX = std::ceil(std::max(width / face.unitsPerLuxel, 1.0f));
    const float luxelsY = std::ceil(std::max(height / face.unitsPerLuxel, 1.0f));
    const stbrp_rect rect = {
        .w = static_cast<stbrp_coord>(luxelsX + LightmapHelpers::LIGHTMAP_PADDING * 2),
        .h = static_cast<stbrp_coord>(luxelsY + LightmapHelpers::LIGHTMAP_PADDING * 2),
    };
    const FaceData faceData = {
        .indices = offsetIndices,
        .positionsInRect = positionsInRect,
        .rect = rect,
    };
    return faceData;
}
