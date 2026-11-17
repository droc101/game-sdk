//
// Created by NBT22 on 3/20/26.
//

#pragma once

#include <cstdint>
#include <libassets/type/Actor.h>
#include <libassets/type/ModelLod.h>
#include <libassets/util/ActorDefinitionManager.h>
#include <libassets/util/SearchPathManager.h>
#include <luna/lunaTypes.h>
#include <vector>
#include <vulkan/vulkan_core.h>
#include "AccelerationStructure.hpp"
#include "BrushMeshBuilder.h"
#include "Light.h"
#include "Lightmap.hpp"
#include "ShaderLight.hpp"
#include "VulkanContext.hpp"

class LightBaker
{
    public:
        static constexpr uint32_t SAMPLES_PER_THREAD = 256;

        static inline bool enableRenderdocCapture = false;
        static inline bool enableNsightCapture = false;

        /// @warning The @c context reference must outlive the constructed @c LightBaker
        LightBaker(VulkanContext &context,
                   const ActorDefinitionManager &definitionManager,
                   const std::vector<BrushMeshBuilder> &meshBuilders,
                   const std::vector<Actor> &actors,
                   const std::vector<Light> &lights,
                   glm::uvec2 lightmapSize,
                   uint32_t bounceCount,
                   uint32_t sampleCount);

        bool Bake(std::vector<uint16_t> &pixelData, std::vector<uint16_t> &indirectLightingPixelData);

        [[nodiscard]] bool IsValid() const;

    private:
        [[nodiscard]] static constexpr bool CheckResult(const VkResult result)
        {
            return VulkanContext::CheckResult(result);
        }

        bool CreateVertexAndIndexBuffers(const std::vector<BrushMeshBuilder> &meshBuilders);

        bool PrecomputeLuxelInformation();

        bool CacheEmissiveLuxelIndices();

        [[nodiscard]] LunaBuffer CreateLodVertexBufferForBLAS(const ModelLod &lod) const;

        /// Create the bottom-level acceleration structures
        bool CreateBLAS(const ActorDefinitionManager &definitionManager,
                        const std::vector<BrushMeshBuilder> &meshBuilders,
                        const std::vector<Actor> &actors);

        /// Create the top-level acceleration structures
        bool CreateTLAS();

        bool CreateAndWriteDescriptorSet();

        bool CreateDirectLightingPipeline(VkBool32 writeDirect);

        bool CreateGlobalIlluminationPipeline();

        bool CreateShaderBindingTables();

        [[nodiscard]] bool SingleBakeIteration(uint32_t width,
                                               uint32_t height,
                                               uint32_t baseLuxelIndex,
                                               uint32_t baseSampleIndex,
                                               bool directLighting) const;

        /// Set to true when successfully constructed
        bool valid = false;

        /// The Vulkan context to use for baking
        VulkanContext &context;
        /// The number of lights
        const uint32_t lightCount;
        /// The size of the lightmap
        const glm::uvec2 lightmapSize;
        /// The number of indirect lighting bounces to use
        const uint32_t bounceCount;
        /// The number of samples to take per luxel per indirect bounce
        const uint32_t sampleCount;

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
        AccelerationStructure tlas;
        uint32_t brushesVertexCount{};
        LunaBuffer brushesVertexBuffer{};
        uint32_t brushesIndexCount{};
        LunaBuffer brushesIndexBuffer{};
        LunaImage luxelPositionsImage{};
        LunaImage luxelNormalsImage{};
        LunaImage luxelAlbedosImage{};
        LunaBuffer lightsBuffer{};
        LunaBuffer emissiveLuxelIndicesBuffer{};
        Lightmap lightmapOne;
        Lightmap lightmapTwo;
        Lightmap lightmapThree;
};
