//
// Created by NBT22 on 3/20/26.
//

#pragma once

#include <glslang/Public/ShaderLang.h>
#include <libassets/util/Logger.h>
#include <luna/lunaTypes.h>
#include <string>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan_core.h>
#include "LevelMeshBuilder.h"
#include "Light.h"

class LightBakerGpu
{
    public:
        LightBakerGpu();

        ~LightBakerGpu();

        bool Bake(const std::unordered_map<std::string, LevelMeshBuilder> &meshBuilders,
                  const std::vector<Light> &lights,
                  const glm::uvec2 &lightmapSize,
                  std::vector<uint8_t> &pixelData);

        [[nodiscard]] bool IsInitialized() const
        {
            return initialized;
        }

    private:
        struct AccelerationStructure
        {
                LunaBuffer buffer;
                VkAccelerationStructureKHR accelerationStructure;
                LunaBuffer scratchBuffer;
        };

        [[nodiscard]] static constexpr bool CheckResult(const VkResult result)
        {
            if (result != VK_SUCCESS)
            {
                Logger::Error("Error {} in Vulkan function!", static_cast<int>(result));
                return false;
            }
            return true;
        }

        VkShaderModule GenerateShaderModule(const std::filesystem::path &path,
                                            EShLanguage shaderType,
                                            std::vector<uint32_t> &spirv) const;

        /// Create the bottom-level acceleration structures
        bool CreateBLAS(const std::unordered_map<std::string, LevelMeshBuilder> &meshBuilders);

        /// Create the top-level acceleration structures
        bool CreateTLAS();

        bool CreateAndWriteDescriptorSet();

        bool CreatePipeline(const glm::uvec2 &lightmapSize, uint32_t lightCount);

        bool CreateShaderBindingTables();

        bool ConvertLightmapToFloat16(const glm::uvec2 &lightmapSize, LunaBuffer &outputLightmap) const;

        static inline VkPhysicalDeviceRayTracingPipelinePropertiesKHR physicalDeviceRayTracingPipelineProperties{
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR,
        };
        static inline VkPhysicalDeviceAccelerationStructurePropertiesKHR physicalDeviceAccelerationStructureProperties{
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR,
            .pNext = &physicalDeviceRayTracingPipelineProperties,
        };
        static inline VkPhysicalDeviceProperties2 physicalDeviceProperties{
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
            .pNext = &physicalDeviceAccelerationStructureProperties,
        };

        bool initialized{};
        LunaDevice device{};
        uint32_t queueFamilyIndex{};
        VkQueue queue{};
        LunaCommandPool commandPool{};
        LunaCommandBuffer commandBuffer{};
        /// The ray tracing pipeline layout
        /// @note This is not managed by Luna because of lack of Luna support for ray tracing extensions
        VkPipelineLayout pipelineLayout{};
        /// The ray tracing pipeline
        /// @note This is not managed by Luna because of lack of Luna support for ray tracing extensions
        VkPipeline pipeline{};
        /// The descriptor set layout
        /// @note This is not managed by Luna because of lack of Luna support for ray tracing extensions
        VkDescriptorSetLayout descriptorSetLayout{};
        /// The descriptor pool
        /// @note This is not managed by Luna because of lack of Luna support for ray tracing extensions
        VkDescriptorPool descriptorPool{};
        /// The descriptor set
        /// @note This is not managed by Luna because of lack of Luna support for ray tracing extensions
        VkDescriptorSet descriptorSet{};
        LunaBuffer raygenShaderBindingTable{};
        LunaBuffer closestHitShaderBindingTable{};
        LunaBuffer missShaderBindingTable{};
        /// The bottom-level acceleration structure used for hardware acceleration of path tracing
        AccelerationStructure blas{};
        /// The buffer that holds the blas instances for the tlas to reference
        LunaBuffer accelerationStructureInstancesBuffer{};
        /// The top-level acceleration structure used for hardware acceleration of path tracing
        AccelerationStructure tlas{};
        LunaBuffer lightsBuffer{};
        LunaBuffer lightmap{};
        LunaBuffer vertexBuffer{};
        LunaBuffer indexBuffer{};
        LunaBuffer lightHitIndicesBuffer{};
        /// The iteration we're on for this light
        uint32_t iteration{};
        /// The index of the light we are drawing rays for. Used as a push constant
        uint32_t lightIndex{};
};
