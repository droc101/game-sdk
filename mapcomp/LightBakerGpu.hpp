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
#include "libassets/type/Actor.h"
#include "Light.h"

class LightBakerGpu
{
    public:
        LightBakerGpu();

        ~LightBakerGpu();

        bool Bake(const std::unordered_map<std::string, LevelMeshBuilder> &meshBuilders,
                  const std::vector<Light> &lights,
                  const glm::uvec2 &lightmapSize,
                  uint32_t bounceCount,
                  uint32_t sampleCount,
                  std::vector<uint16_t> &pixelData);

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

        VkShaderModule GenerateShaderModule(const std::filesystem::path &path, EShLanguage shaderType) const;

        bool CreateVertexAndIndexBuffers(const std::unordered_map<std::string, LevelMeshBuilder> &meshBuilders,
                                         uint32_t &vertexCount,
                                         uint32_t &indexCount);

        bool PrecomputeLuxelInformation(const glm::uvec2 &lightmapSize, uint32_t indexCount);

        /// Create the bottom-level acceleration structures
        bool CreateBLAS(uint32_t vertexCount, uint32_t indexCount);

        /// Create the top-level acceleration structures
        bool CreateTLAS();

        bool CreateAndWriteDescriptorSet();

        bool CreateDirectLightingPipeline(const glm::uvec2 &lightmapSize, uint32_t lightCount);

        bool CreateGlobalIlluminationPipeline(const glm::uvec2 &lightmapSize, uint32_t sampleCount);

        bool CreateShaderBindingTables();

        bool SingleBakeIteration(uint64_t width,
                                 uint64_t height,
                                 float percentDone,
                                 uint32_t baseLuxelIndex,
                                 bool directLighting) const;

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
        LunaSemaphore semaphore{};
        /// The ray tracing pipeline layout
        /// @note This is not managed by Luna because of lack of Luna support for ray tracing extensions
        VkPipelineLayout pipelineLayout{};
        /// The ray tracing pipeline used for calculating direct lighting
        /// @note This is not managed by Luna because of lack of Luna support for ray tracing extensions
        VkPipeline directLightingPipeline{};
        /// The ray tracing pipeline used for calculating global illumination
        /// @note This is not managed by Luna because of lack of Luna support for ray tracing extensions
        VkPipeline globalIlluminationPipeline{};
        /// The descriptor set layout
        /// @note This is not managed by Luna because of lack of Luna support for ray tracing extensions
        VkDescriptorSetLayout descriptorSetLayout{};
        /// The descriptor pool
        /// @note This is not managed by Luna because of lack of Luna support for ray tracing extensions
        VkDescriptorPool descriptorPool{};
        /// The descriptor set
        /// @note This is not managed by Luna because of lack of Luna support for ray tracing extensions
        VkDescriptorSet descriptorSet{};
        LunaBuffer directLightingRaygenShaderBindingTable{};
        LunaBuffer missShaderBindingTable{};
        LunaBuffer globalIlluminationRaygenShaderBindingTable{};
        LunaBuffer closestHitShaderBindingTable{};
        /// The bottom-level acceleration structure used for hardware acceleration of path tracing
        AccelerationStructure blas{};
        /// The buffer that holds the blas instances for the tlas to reference
        LunaBuffer accelerationStructureInstancesBuffer{};
        /// The top-level acceleration structure used for hardware acceleration of path tracing
        AccelerationStructure tlas{};
        LunaBuffer vertexBuffer{};
        LunaBuffer indexBuffer{};
        LunaImage luxelPositionsImage{};
        LunaImage luxelNormalsImage{};
        LunaImage luxelAlbedosImage{};
        LunaBuffer lightsBuffer{};
        LunaBuffer lightmapOne{};
        LunaBuffer lightmapTwo{};
        VkBufferView lightmapOneBufferView{};
        VkBufferView lightmapTwoBufferView{};
};
