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
        using Triangle = std::array<MapVertex, 3>;

    public:
        LightBakerCpu() = delete;
        LightBakerCpu(const std::unordered_map<std::string, LevelMeshBuilder> &meshBuilders,
                      const std::vector<Light> &lights,
                      const glm::uvec2 &lightmapSize);

        void Bake(uint64_t rayCount, uint32_t bounceCount, std::vector<uint16_t> &pixelData);

    private:
        std::vector<Triangle> triangles;
        std::vector<Light> lights;
        glm::uvec2 lightmapSize;
};
