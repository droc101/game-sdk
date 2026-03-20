//
// Created by NBT22 on 3/11/26.
//

#pragma once

#include <cstddef>
#include <cstdint>
#include <luna/lunaTypes.h>
#include <string>
#include <unordered_map>
#include <vector>
#include "LevelMeshBuilder.h"
#include "libassets/type/Actor.h"
#include "Light.h"

class LightBaker
{
    public:
        LightBaker();

        ~LightBaker();

        bool bake(const std::unordered_map<std::string, LevelMeshBuilder> &meshBuilders,
                  const std::vector<Light> &lights,
                  std::vector<uint8_t> &pixelData,
                  const glm::ivec2 &lightmapSize);

        static bool bakeCPU(const std::unordered_map<std::string, LevelMeshBuilder> &meshBuilders,
                     const std::vector<Light> &lights,
                     std::vector<uint8_t> &pixelData,
                     const glm::ivec2 &lightmapSize);

        [[nodiscard]] bool IsInitialized() const
        {
            return initialized;
        }

    private:
        static constexpr uint32_t LIGHTMAP_3D_SIZE = 1 << 14;

        bool initialized{};

        LunaDescriptorSetLayout descriptorSetLayout{};
        LunaDescriptorPool descriptorPool{};
        LunaDescriptorSet descriptorSet{};
        LunaBuffer vertexBuffer{};
        LunaBuffer indexBuffer{};
        LunaBuffer levelGeometryLightmap{};
        LunaShaderModule shaderModule{};
        LunaComputePipeline pipeline{};
        LunaImage lightmap3d{};
};
