//
// Created by NBT22 on 3/11/26.
//

#pragma once

#include <cstdint>
#include <vector>
#include "BrushMeshBuilder.h"
#include "Light.h"

class LightBaker
{
    public:
        static bool Bake(const std::vector<BrushMeshBuilder> &meshBuilders,
                         const std::vector<Light> &lights,
                         const glm::uvec2 &lightmapSize,
                         std::vector<uint16_t> &pixelData,
                         std::vector<uint16_t> &indirectLightingPixelData);
};
