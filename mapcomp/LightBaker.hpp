//
// Created by NBT22 on 3/11/26.
//

#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>
#include "LevelMeshBuilder.h"
#include "Light.h"

class LightBaker
{
    public:
        static bool Bake(const std::unordered_map<std::string, LevelMeshBuilder> &meshBuilders,
                         const std::vector<Light> &lights,
                         std::vector<uint8_t> &pixelData,
                         const glm::uvec2 &lightmapSize,
                         bool useCpu);
};
