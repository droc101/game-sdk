//
// Created by NBT22 on 3/20/26.
//

#include "LightBakerCpu.hpp"
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <glm/geometric.hpp>
#include <libassets/type/MapVertex.h>
#include <limits>
#include <numbers>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>
#include "LevelMeshBuilder.h"
#include "libassets/type/Actor.h"
#include "Light.h"

LightBakerCpu::LightBakerCpu(const std::unordered_map<std::string, LevelMeshBuilder> &meshBuilders,
                             const std::vector<Light> &lights,
                             const glm::uvec2 &lightmapSize):
    lights(lights),
    lightmapSize(lightmapSize)
{
    size_t indexOffset = 0;
    std::vector<MapVertex> vertices{};
    std::vector<uint32_t> indices{};
    for (const LevelMeshBuilder &builder: meshBuilders | std::views::values)
    {
        vertices.insert(vertices.end(), builder.GetVertices().begin(), builder.GetVertices().end());
        for (const uint32_t index: builder.GetIndices())
        {
            indices.emplace_back(index + indexOffset);
        }
        indexOffset += builder.GetVertices().size();
    }

    triangles.reserve(indices.size() / 3);
    for (size_t i = 0; i < indices.size() / 3; i += 3)
    {
        triangles.emplace_back(std::array<MapVertex, 3>{vertices.at(i), vertices.at(i + 1), vertices.at(i + 2)});
    }
}

void LightBakerCpu::Bake(const uint64_t rayCount, const uint32_t bounceCount, std::vector<uint16_t> &pixelData)
{

}

void CastRay(const Light &source, const glm::vec3 &direction, std::vector<uint16_t> &pixelData) {}
