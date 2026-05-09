//
// Created by NBT22 on 3/20/26.
//

#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>
#include "LevelMeshBuilder.h"
#include "Light.h"

class LightBakerCpu
{
    public:
        static void Bake(const std::unordered_map<std::string, LevelMeshBuilder> &meshBuilders,
                         const std::vector<Light> &lights,
                         std::vector<uint8_t> &pixelData,
                         const glm::uvec2 &lightmapSize);
};
