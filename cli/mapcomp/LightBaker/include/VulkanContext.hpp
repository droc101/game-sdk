//
// Created by NBT22 on 11/16/26.
//

#pragma once

#include <cstdint>
#include <filesystem>
#include <glslang/Public/ShaderLang.h>
#include <libassets/util/Logger.h>
#include <libassets/util/SearchPathManager.h>
#include <luna/lunaTypes.h>
#include <string>
#include <utility>
#include <vector>
#include <vulkan/vulkan_core.h>

class VulkanContext
{
        friend class AccelerationStructure;
        friend class Lightmap;
        friend class LightBaker;

    public:
        /// @warning The @c pathManager reference must outlive the constructed @c VulkanContext
        VulkanContext(const SearchPathManager &pathManager, bool enableNsightCapture);
        ~VulkanContext();

        bool GetTextureIndex(const std::string &texture, uint32_t &index);
        bool GetMaterialTextureIndex(const std::string &materialName, uint32_t &index);

    private:
        [[nodiscard]] static constexpr bool CheckResult(const VkResult result)
        {
            if (result != VK_SUCCESS)
            {
                Logger::Error("Error {} in Vulkan function!", static_cast<int>(result));
                return false;
            }
            return true;
        }


#ifdef NDEBUG
        static constexpr bool SHADER_DEBUG_INFO = false;
#else
        static constexpr bool SHADER_DEBUG_INFO = true;
#endif

        [[nodiscard]] VkShaderModule GenerateShaderModule(const std::filesystem::path &path,
                                                          EShLanguage shaderType) const;

        [[nodiscard]] bool SubmitFrameBoundary();

        const SearchPathManager &pathManager;
        bool valid{};
        bool frameBoundaryEnabled{};
        uint64_t currentFrameId{};
        LunaDevice device{};
        uint32_t queueFamilyIndex{};
        VkQueue queue{};
        LunaCommandPool commandPool{};
        LunaCommandBuffer commandBuffer{};
        LunaSemaphore semaphore{};
        LunaCommandBufferSubmitInfo submitInfo{};
        std::vector<std::pair<std::string, LunaImage>> loadedTextures{};
        VkDescriptorPool texturesDescriptorPool{};
        VkDescriptorSetLayout texturesDescriptorSetLayout{};
        VkDescriptorSet texturesDescriptorSet{};

        VkPhysicalDeviceRayTracingPipelinePropertiesKHR physicalDeviceRayTracingPipelineProperties{
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR,
        };
        VkPhysicalDeviceAccelerationStructurePropertiesKHR physicalDeviceAccelerationStructureProperties{
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR,
            .pNext = &physicalDeviceRayTracingPipelineProperties,
        };
        VkPhysicalDeviceProperties2 physicalDeviceProperties{
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
            .pNext = &physicalDeviceAccelerationStructureProperties,
        };
};
