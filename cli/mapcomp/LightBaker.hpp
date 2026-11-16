//
// Created by NBT22 on 3/20/26.
//

#pragma once

#include <cstdint>
#include <filesystem>
#include <glslang/Public/ShaderLang.h>
#include <libassets/type/Actor.h>
#include <libassets/type/ModelLod.h>
#include <libassets/util/ActorDefinitionManager.h>
#include <libassets/util/Logger.h>
#include <libassets/util/SearchPathManager.h>
#include <luna/lunaTypes.h>
#include <string>
#include <utility>
#include <vector>
#include <vulkan/vulkan_core.h>
#include "BrushMeshBuilder.h"
#include "Light.h"

class LightBaker
{
        struct Lightmap
        {
                LunaBuffer buffer;
                VkBufferView bufferView;
        };

        struct ShaderLight
        {
                Light::Type type;
                glm::vec3 position;
                glm::vec3 cookieAxisV;
                glm::vec3 negativeForwardDirection;
                glm::vec3 color;
                float brightness;
                float constantAttenuation;
                float linearAttenuation;
                float quadraticAttenuation;
                float attenuationMultiplier;
                float brightAngle;
                float fadingAngle;
                uint32_t cookieTextureIndex;
        };

    public:
        struct BakeSettings
        {
                const glm::uvec2 &lightmapSize;
                uint32_t bounceCount;
                uint32_t sampleCount;
                const std::vector<BrushMeshBuilder> &meshBuilders;
                const std::vector<Actor> &actors;
                const std::vector<Light> &lights;
        };

        static inline bool enableRenderdocCapture = false;
        static constexpr uint32_t SAMPLES_PER_THREAD = 256;

        static LightBaker &Get();

        bool Bake(const SearchPathManager &pathManager,
                  const ActorDefinitionManager &definitionManager,
                  const BakeSettings &settings,
                  std::vector<uint16_t> &pixelData,
                  std::vector<uint16_t> &indirectLightingPixelData);

        bool GetTextureIndex(const std::string &texture, uint32_t &index, const SearchPathManager &pathManager);
        bool GetMaterialTextureIndex(const std::string &materialName,
                                     uint32_t &index,
                                     const SearchPathManager &pathManager);

    private:
        LightBaker();

        ~LightBaker();

        struct AccelerationStructure
        {
                VkAccelerationStructureInstanceKHR instance;
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

        [[nodiscard]] VkShaderModule GenerateShaderModule(const std::filesystem::path &path,
                                                          EShLanguage shaderType) const;

        VkResult CreateBufferView(LunaBuffer buffer, VkFormat format, VkBufferView &bufferView) const;

        bool CreateLightmap(Lightmap &lightmap, const glm::uvec2 &lightmapSize) const;

        bool CreateVertexAndIndexBuffers(const std::vector<BrushMeshBuilder> &meshBuilders,
                                         uint32_t &vertexCount,
                                         uint32_t &indexCount);

        bool PrecomputeLuxelInformation(const glm::uvec2 &lightmapSize, uint32_t indexCount);

        bool CacheEmissiveLuxelIndices(const glm::uvec2 &lightmapSize);

        [[nodiscard]] LunaBuffer CreateLodVertexBufferForBLAS(const ModelLod &lod) const;

        /// Create a single bottom-level acceleration structure
        template<typename VertexType> bool CreateBLAS(const VkDeviceOrHostAddressConstKHR &vertexDataAddress,
                                                      const VkDeviceOrHostAddressConstKHR &indexDataAddress,
                                                      uint32_t triangleCount,
                                                      uint32_t maxVertex,
                                                      const VkTransformMatrixKHR &transform,
                                                      uint32_t firstIndex);

        /// Create the bottom-level acceleration structures
        bool CreateBLAS(const SearchPathManager &pathManager,
                        const ActorDefinitionManager &definitionManager,
                        const std::vector<BrushMeshBuilder> &meshBuilders,
                        const std::vector<Actor> &actors,
                        uint32_t vertexCount);

        /// Create the top-level acceleration structures
        bool CreateTLAS();

        bool CreateAndWriteDescriptorSet();

        bool CreateDirectLightingPipeline(const glm::uvec2 &lightmapSize, uint32_t lightCount, VkBool32 writeDirect);

        bool CreateGlobalIlluminationPipeline(const glm::uvec2 &lightmapSize, uint32_t sampleCount);

        bool CreateShaderBindingTables();

        [[nodiscard]] bool SingleBakeIteration(uint32_t width,
                                               uint32_t height,
                                               uint32_t baseLuxelIndex,
                                               uint32_t baseSampleIndex,
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

        static inline std::vector<std::pair<std::string, LunaImage>> loadedTextures{};

        bool initialized{};
        LunaDevice device{};
        uint32_t queueFamilyIndex{};
        VkQueue queue{};
        LunaCommandPool commandPool{};
        LunaCommandBuffer commandBuffer{};
        LunaSemaphore semaphore{};
        LunaCommandBufferSubmitInfo submitInfo{};
        VkDescriptorPool graphicsDescriptorPool{};
        VkDescriptorSetLayout graphicsDescriptorSetLayout{};
        VkDescriptorSet graphicsDescriptorSet{};
        /// The ray tracing pipeline layout
        /// @note This is not managed by Luna because of lack of Luna support for ray tracing extensions
        VkPipelineLayout directLightingPipelineLayout{};
        /// The ray tracing pipeline used for calculating direct lighting
        /// @note This is not managed by Luna because of lack of Luna support for ray tracing extensions
        VkPipeline directLightingPipeline{};
        /// The ray tracing pipeline layout
        /// @note This is not managed by Luna because of lack of Luna support for ray tracing extensions
        VkPipelineLayout globalIlluminationPipelineLayout{};
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
        /// A shader binding table containing a single zeroed record, used for unused shader binding table regions
        LunaBuffer nullShaderBindingTable{};
        /// The bottom-level acceleration structures used for hardware acceleration of path tracing
        std::vector<AccelerationStructure> bottomLevelAccelerationStructures{};
        /// The buffer that holds the blas instances for the tlas to reference
        LunaBuffer accelerationStructureInstancesBuffer{};
        /// The top-level acceleration structure used for hardware acceleration of path tracing
        AccelerationStructure tlas{};
        LunaBuffer brushesVertexBuffer{};
        LunaBuffer brushesIndexBuffer{};
        LunaImage luxelPositionsImage{};
        LunaImage luxelNormalsImage{};
        LunaImage luxelAlbedosImage{};
        LunaBuffer lightsBuffer{};
        LunaBuffer emissiveLuxelIndicesBuffer{};
        Lightmap lightmapOne{};
        Lightmap lightmapTwo{};
        Lightmap lightmapThree{};
};
