//
// Created by NBT22 on 3/20/26.
//

#pragma once

#include <array>
#include <cstdint>
#include <libassets/util/Logger.h>
#include <luna/lunaTypes.h>
#include <string>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan_core.h>
#include "LevelMeshBuilder.h"
#include "libassets/type/Actor.h"
#include "Light.h"

class LightBakerGpu
{
    public:
        LightBakerGpu();

        ~LightBakerGpu();

        bool bake(const std::unordered_map<std::string, LevelMeshBuilder> &meshBuilders,
                  const std::vector<Light> &lights,
                  const glm::ivec2 &lightmapSize,
                  std::vector<uint8_t> &pixelData);

        [[nodiscard]] bool isInitialized() const
        {
            return initialized;
        }

    private:
        [[nodiscard]] static constexpr bool checkResult(const VkResult result)
        {
            if (result != VK_SUCCESS)
            {
                Logger::Error("Error {} in Vulkan function!", static_cast<uint32_t>(result));
                return false;
            }
            return true;
        }

        static bool createPipeline(const std::string &shader,
                                   const VkSpecializationInfo &specializationInfo,
                                   const std::vector<LunaPushConstantsRange> &pushConstantRanges,
                                   const LunaDescriptorSetLayoutCreationInfo &descriptorSetLayoutCreationInfo,
                                   const LunaDescriptorPoolCreationInfo &descriptorPoolCreationInfo,
                                   LunaDescriptorSet &descriptorSet,
                                   LunaComputePipeline &pipeline);

        bool bakeDirectLighting(const std::vector<Light> &lights,
                                const glm::ivec2 &lightmapSize,
                                size_t indexCount) const;

        bool bakeBounceLighting(const glm::ivec2 &lightmapSize, size_t indexCount, uint32_t bounceCount);

        bool convertLightmapToFloat16(const glm::ivec2 &lightmapSize, LunaBuffer &outputLightmap2d) const;

        bool initialized{};
        LunaBuffer vertexBuffer{};
        LunaBuffer indexBuffer{};
        std::array<LunaBuffer, 2> pixelIndicesBuffers{};
        LunaBuffer hitIndicesBuffer{};
        LunaBuffer lightmap2d{};
};
