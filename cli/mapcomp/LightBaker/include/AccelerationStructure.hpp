//
// Created by NBT22 on 11/16/26.
//

#pragma once

#include <cstdint>
#include <functional>
#include <luna/lunaTypes.h>
#include <vulkan/vulkan_core.h>
#include "VulkanContext.hpp"

class AccelerationStructure
{
    public:
        /// @warning The @c context reference must outlive the constructed @c AccelerationStructure
        explicit AccelerationStructure(const VulkanContext &context);

        /// @warning The @c context reference must outlive the constructed @c AccelerationStructure
        AccelerationStructure(const VulkanContext &context,
                              const std::vector<AccelerationStructure> &bottomLevelAccelerationStructures);

        /// @warning The @c context reference must outlive the constructed @c AccelerationStructure
        AccelerationStructure(const VulkanContext &context,
                              const VkAccelerationStructureGeometryTrianglesDataKHR &geometryTrianglesData,
                              uint32_t triangleCount,
                              const VkTransformMatrixKHR &transform,
                              uint32_t firstIndex,
                              uint8_t mask);

        void WriteToDescriptorSet(const VkDescriptorSet &descriptorSet, uint32_t bindingIndex) const;

        [[nodiscard]] bool IsValid() const;

        [[nodiscard]] const VkAccelerationStructureInstanceKHR &InstanceData() const;

    private:
        std::reference_wrapper<const VulkanContext> context;
        VkAccelerationStructureInstanceKHR instance{};
        LunaBuffer buffer{};
        VkAccelerationStructureKHR accelerationStructure{};
        LunaBuffer scratchBuffer{};
        /// The instances buffer for the TLAS. Only valid on a TLAS
        LunaBuffer instancesBuffer{};
};
