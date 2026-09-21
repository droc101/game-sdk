//
// Created by NBT22 on 3/11/26.
//

#include "LightBaker.hpp"
#include <cstdint>
#include <libassets/type/MapVertex.h>
#include <type_traits>
#include <vector>
#include "LevelMeshBuilder.h"
#include "Light.h"
#include "LightBakerGpu.hpp"

bool LightBaker::Bake(const std::vector<LevelMeshBuilder> &meshBuilders,
                      const std::vector<Light> &lights,
                      const glm::uvec2 &lightmapSize,
                      std::vector<uint16_t> &pixelData,
                      std::vector<uint16_t> &indirectLightingPixelData)
{
    static constexpr uint32_t BOUNCE_COUNT = 3;
    static constexpr uint32_t SAMPLE_COUNT = 8192;

    return LightBakerGpu::Get()
            .Bake(meshBuilders, lights, lightmapSize, BOUNCE_COUNT, SAMPLE_COUNT, pixelData, indirectLightingPixelData);
}
