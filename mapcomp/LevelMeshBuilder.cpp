//
// Created by droc101 on 11/16/25.
//

#include "LevelMeshBuilder.h"
#include <array>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <glm/detail/func_geometric.inl>
#include <glm/vec2.hpp>
#include <libassets/type/MapVertex.h>
#include <libassets/type/Sector.h>
#include <libassets/type/WallMaterial.h>
#include <libassets/util/DataWriter.h>
#include <libassets/util/Logger.h>
#include <libassets/util/SearchPathManager.h>
#include <ranges>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include "SectorClipper.h"

#define STB_RECT_PACK_IMPLEMENTATION
#include <stb_rect_pack.h>

#include "libassets/asset/LevelMaterialAsset.h"

void LevelMeshBuilder::AddCeiling(const Sector &sector, const std::vector<const Sector *> &overlapping)
{
    AddSectorBase(sector, false, overlapping);
}

void LevelMeshBuilder::AddFloor(const Sector &sector, const std::vector<const Sector *> &overlapping)
{
    AddSectorBase(sector, true, overlapping);
}

void LevelMeshBuilder::AddWall(const Sector &sector,
                               const size_t wallIndex,
                               const float floorHeight,
                               const float ceilingHeight)
{
    const float sLength = CalculateSLength(sector, wallIndex);

    const WallMaterial &mat = sector.wallMaterials.at(wallIndex);

    const glm::vec2 &startPoint = sector.points.at(wallIndex);
    const glm::vec2 &endPoint = sector.points.at((wallIndex + 1) % (sector.points.size()));

    const glm::vec2 normal = sector.SegmentNormal(wallIndex);
    const bool ccw = sector.CalculateArea() > 0;

    AddWallBase(startPoint, endPoint, mat, normal, sLength, floorHeight, ceilingHeight, ccw);
}

void LevelMeshBuilder::Write(DataWriter &writer, const std::string &materialPath) const
{
    writer.WriteString(materialPath);
    writer.Write<uint32_t>(vertices.size());
    for (const MapVertex &vertex: vertices)
    {
        writer.WriteVec3(vertex.position);
        writer.WriteVec2(vertex.uv);
        writer.WriteVec2(vertex.lightmapUv);
    }
    writer.Write<uint32_t>(indices.size());
    writer.WriteBuffer<uint32_t>(indices);
}

bool LevelMeshBuilder::IsEmpty() const
{
    return vertices.empty() || indices.size() < 3;
}

// TODO find better place for this
static float mapRange(const float value,
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

bool LevelMeshBuilder::CalculateLightmapUvs(glm::uvec2 &lightmapSize,
                                            std::unordered_map<std::string, LevelMeshBuilder> &meshBuilders,
                                            const SearchPathManager &pathMgr)
{
    for (const LevelMeshBuilder &builder: meshBuilders | std::views::values)
    {
        assert(builder.faceIndices.size() == builder.faceRects.size());
    }

    int width = 1 << 4;
    int height = 1 << 4;
    bool wasHeightChangedLast = true;
    constexpr int maxSize = 1 << 14;

    stbrp_context context{};

    std::vector<stbrp_rect> rects{};
    for (const std::pair<const std::string, LevelMeshBuilder> &builder: meshBuilders)
    {
        const std::string materialPath = pathMgr.GetAssetPath(builder.first);
        LevelMaterialAsset material{};
        LevelMaterialAsset::CreateFromAsset(materialPath.c_str(), material);
        if (material.shader == Material::MaterialShader::SHADER_SHADED)
        {
            rects.insert(rects.end(), builder.second.faceRects.begin(), builder.second.faceRects.end());
        }
    }

    std::vector<stbrp_node> nodes{};
    while (true)
    {
        nodes.clear();
        nodes.resize(width * 2);
        stbrp_init_target(&context, width, height, nodes.data(), static_cast<int>(nodes.size()));

        if (stbrp_pack_rects(&context, rects.data(), static_cast<int>(rects.size())) == 0)
        {
            // printf("Lightmap size of %d by %d didn't fit :(\n", width, height);
            if (width == maxSize && height == maxSize)
            {
                return false;
            }
            if (wasHeightChangedLast)
            {
                width = width << 1;
                wasHeightChangedLast = false;
            } else
            {
                height = height << 1;
                wasHeightChangedLast = true;
            }
        } else
        {
            break;
        }
    }

    Logger::Info("Lightmap size: {} by {}", width, height);

    lightmapSize = {width, height};

    size_t rectIndexBegin = 0;
    for (std::pair<const std::string, LevelMeshBuilder> &builder: meshBuilders)
    {
        const std::string materialPath = pathMgr.GetAssetPath(builder.first);
        LevelMaterialAsset material{};
        LevelMaterialAsset::CreateFromAsset(materialPath.c_str(), material);
        if (material.shader != Material::MaterialShader::SHADER_SHADED)
        {
            continue;
        }

        for (size_t i = 0; i < builder.second.faceIndices.size(); i++)
        {
            const stbrp_rect &rect = rects.at(i + rectIndexBegin);
            const FaceData &faceData = builder.second.faceIndices.at(i);
            assert(rect.h != 0);
            assert(rect.w != 0);
            for (size_t j = 0; j < faceData.indices.size(); j++)
            {
                const uint32_t index = faceData.indices.at(j);
                MapVertex &vertex = builder.second.vertices.at(index);
                const glm::vec2 &positionInRect = faceData.positionsInRect.at(j);
                vertex.lightmapUv.x = mapRange(positionInRect.x,
                                               0.0f,
                                               1.0f,
                                               static_cast<float>(rect.x) + LIGHTMAP_PADDING,
                                               static_cast<float>(rect.x + rect.w) - LIGHTMAP_PADDING) /
                                      static_cast<float>(width);
                vertex.lightmapUv.y = mapRange(positionInRect.y,
                                               0.0f,
                                               1.0f,
                                               static_cast<float>(rect.y) + LIGHTMAP_PADDING,
                                               static_cast<float>(rect.y + rect.h) - LIGHTMAP_PADDING) /
                                      static_cast<float>(height);
            }
        }
        rectIndexBegin += builder.second.faceRects.size();
    }

    return true;
}

float LevelMeshBuilder::CalculateSLength(const Sector &sector, const size_t wallIndex)
{
    float sLength = 0;
    for (size_t i = 0; i < wallIndex; i++)
    {
        const glm::vec2 &startPoint = sector.points.at(i);
        const glm::vec2 &endPoint = sector.points.at((i + 1) % (sector.points.size()));
        sLength += glm::distance(startPoint, endPoint);
    }
    return sLength;
}

void LevelMeshBuilder::AddWallBase(const glm::vec2 &startPoint,
                                   const glm::vec2 &endPoint,
                                   const WallMaterial &wallMaterial,
                                   const glm::vec2 wallNormalVector,
                                   const float previousWallsLength,
                                   const float floorHeight,
                                   const float ceilingHeight,
                                   const bool counterClockWise)
{
    if (floorHeight > ceilingHeight)
    {
        Logger::Warning("Wall with ceiling below floor, will be skipped");
        return;
    }

    if (floorHeight == ceilingHeight)
    {
        return;
    }

    std::array<glm::vec3, 4> wallPoints{};
    wallPoints.at(0) = {startPoint.x, ceilingHeight, startPoint.y}; // SC
    wallPoints.at(1) = {endPoint.x, ceilingHeight, endPoint.y}; // EC

    wallPoints.at(2) = {startPoint.x, floorHeight, startPoint.y}; // SF
    wallPoints.at(3) = {endPoint.x, floorHeight, endPoint.y}; // EF

    const glm::vec2 startPointV = {startPoint.x, startPoint.y};
    const glm::vec2 endPointV = {endPoint.x, endPoint.y};
    const float wallLength = glm::distance(startPointV, endPointV);

    for (const glm::vec3 &point: wallPoints)
    {
        MapVertex v{};

        v.position = point;

        v.uv.x = previousWallsLength;
        if (point.x == endPoint.x && point.z == endPoint.y)
        {
            v.uv.x += wallLength;
        }
        v.uv.y = -point.y;

        v.uv += wallMaterial.uvOffset;
        v.uv *= wallMaterial.uvScale; // TODO is this the correct way to offset+scale?

        v.normal.x = -wallNormalVector.x;
        v.normal.y = 0;
        v.normal.z = -wallNormalVector.y;

        v.lightmapUv = glm::vec2(0, 0);

        vertices.push_back(v);
    }

    std::vector<uint32_t> newIndices;
    if (counterClockWise)
    {
        newIndices = {
            2 + currentIndex,
            1 + currentIndex,
            0 + currentIndex,
            1 + currentIndex,
            2 + currentIndex,
            3 + currentIndex,
        };
    } else
    {
        newIndices = {
            0 + currentIndex,
            1 + currentIndex,
            2 + currentIndex,
            3 + currentIndex,
            2 + currentIndex,
            1 + currentIndex,
        };
    }
    indices.insert(indices.end(), newIndices.begin(), newIndices.end());

    const float width = glm::distance(startPoint, endPoint);
    const float height = ceilingHeight - floorHeight;
    std::vector<glm::vec2> positionsInRect{};
    for (const uint32_t index: newIndices)
    {
        switch (index - currentIndex)
        {
            case 0:
                positionsInRect.emplace_back(0, 0); // start ceiling
                break;
            case 1:
                positionsInRect.emplace_back(1, 0); // end ceiling
                break;
            case 2:
                positionsInRect.emplace_back(0, 1); // start floor
                break;
            case 3:
                positionsInRect.emplace_back(1, 1); // end floor
                break;
            default:
                assert(false);
        }
    }
    assert(newIndices.size() == positionsInRect.size());
    faceIndices.push_back({
        .indices = newIndices,
        .positionsInRect = positionsInRect,
    });
    const float luxelsX = fmaxf(width * static_cast<float>(wallMaterial.luxelsPerUnit), 1.0f);
    const float luxelsY = fmaxf(height * static_cast<float>(wallMaterial.luxelsPerUnit), 1.0f);
    faceRects.emplace_back(0, luxelsX + LIGHTMAP_PADDING * 2, luxelsY + LIGHTMAP_PADDING * 2);

    currentIndex += 4;
}

void LevelMeshBuilder::AddSectorBase(const Sector &sector,
                                     const bool isFloor,
                                     const std::vector<const Sector *> &overlapping)
{
    SectorClipper clipper = SectorClipper(sector);
    for (const Sector *s: overlapping)
    {
        clipper.AddHole(s);
    }
    std::vector<glm::vec2> points{};
    std::vector<uint32_t> idx{};
    clipper.ProcessAndMesh(points, idx);

    const WallMaterial &mat = isFloor ? sector.floorMaterial : sector.ceilingMaterial;

    for (const glm::vec2 &point: points)
    {
        MapVertex v{};
        v.position = {point.x, isFloor ? sector.floorHeight : sector.ceilingHeight, point.y};
        v.uv = (point + mat.uvOffset) * mat.uvScale; // TODO is this the correct way to offset+scale?
        v.normal = {0, isFloor ? 1 : -1, 0};
        v.lightmapUv = glm::vec2(0, 0);
        vertices.push_back(v);
    }

    if (isFloor)
    {
        for (size_t i = 0; i + 2 < idx.size(); i += 3)
        {
            std::swap(idx.at(i), idx.at(i + 2));
        }
    }

    for (uint32_t &i: idx)
    {
        i += currentIndex;
    }
    indices.insert(indices.end(), idx.begin(), idx.end());

    if (!points.empty())
    {
        const glm::vec4 sectorAABB = sector.GetAABB();
        const bool rotate = sectorAABB.z < sectorAABB.w;
        const glm::vec2 sectorTopLeft = {sectorAABB.x - sectorAABB.z, sectorAABB.y - sectorAABB.w};
        const glm::vec2 sectorBottomRight = {sectorAABB.x + sectorAABB.z, sectorAABB.y + sectorAABB.w};
        std::vector<glm::vec2> positionsInRect{};
        for (const uint32_t index: idx)
        {
            const MapVertex &vert = vertices.at(index);
            const glm::vec2 position = {vert.position.x, vert.position.z};
            float localX = mapRange(position.x, sectorTopLeft.x, sectorBottomRight.x, 0.0f, 1.0f);
            float localY = mapRange(position.y, sectorTopLeft.y, sectorBottomRight.y, 0.0f, 1.0f);
            if (rotate)
            {
                std::swap(localX, localY);
            }
            positionsInRect.emplace_back(localX, localY);
        }

        float width = sectorAABB.z * 2;
        float height = sectorAABB.w * 2;
        if (rotate)
        {
            std::swap(width, height);
        }
        const float luxelsPerUnit = isFloor ? sector.floorMaterial.luxelsPerUnit : sector.ceilingMaterial.luxelsPerUnit;
        faceIndices.emplace_back(idx, positionsInRect);
        const float luxelsX = fmaxf(width * static_cast<float>(luxelsPerUnit), 1.0f);
        const float luxelsY = fmaxf(height * static_cast<float>(luxelsPerUnit), 1.0f);
        const stbrp_rect rect = {
            .id = 0,
            .w = static_cast<stbrp_coord>(luxelsX + LIGHTMAP_PADDING * 2),
            .h = static_cast<stbrp_coord>(luxelsY + LIGHTMAP_PADDING * 2),
        };
        faceRects.push_back(rect);
    }

    currentIndex += points.size();
}
