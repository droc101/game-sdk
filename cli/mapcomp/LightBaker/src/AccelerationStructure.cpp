//
// Created by NBT22 on 11/16/26.
//

#include "AccelerationStructure.hpp"
#include <cstdint>
#include <luna/lunaBuffer.h>
#include <luna/lunaCommandBuffer.h>
#include <luna/lunaDevice.h>
#include <luna/lunaTypes.h>
#include <vector>
#include <volk.h>
#include <vulkan/vulkan_core.h>
#include "VulkanContext.hpp"

AccelerationStructure::AccelerationStructure(const VulkanContext &context): context(context) {}

AccelerationStructure::AccelerationStructure(const VulkanContext &context,
                                             const std::vector<AccelerationStructure>
                                                     &bottomLevelAccelerationStructures):
    context(context)
{
    const LunaBufferCreationInfo instancesBufferCreationInfo = {
        .size = bottomLevelAccelerationStructures.size() * sizeof(VkAccelerationStructureInstanceKHR),
        .alignment = 16, // 16 is directly required by the spec
        .memoryType = LUNA_MEMORY_TYPE_DEVICE_LOCAL,
        .additionalUsageFlags = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
                                VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
    };
    if (!VulkanContext::CheckResult(lunaCreateBuffer(context.device, &instancesBufferCreationInfo, &instancesBuffer)))
    {
        instancesBuffer = LUNA_NULL_HANDLE;
        return;
    }

    std::vector<VkAccelerationStructureInstanceKHR> instances;
    instances.reserve(bottomLevelAccelerationStructures.size());
    for (const AccelerationStructure &blas: bottomLevelAccelerationStructures)
    {
        instances.emplace_back(blas.InstanceData());
    }
    const LunaBufferWriteInfo instancesBufferWriteInfo = {
        .size = instances.size() * sizeof(VkAccelerationStructureInstanceKHR),
        .data = instances.data(),
        .sourceStageMask = VK_PIPELINE_STAGE_2_NONE,
        .sourceAccessMask = VK_ACCESS_2_NONE,
        .destinationStageMask = VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
        .destinationAccessMask = VK_ACCESS_2_ACCELERATION_STRUCTURE_READ_BIT_KHR |
                                 VK_ACCESS_2_ACCELERATION_STRUCTURE_WRITE_BIT_KHR,
    };
    if (!VulkanContext::CheckResult(lunaWriteDataToBuffer(context.device,
                                                          context.commandBuffer,
                                                          instancesBuffer,
                                                          &instancesBufferWriteInfo)))
    {
        lunaDestroyBuffer(instancesBuffer);
        instancesBuffer = LUNA_NULL_HANDLE;
        return;
    }

    const VkDeviceOrHostAddressConstKHR instancesDataAddress = {
        .deviceAddress = lunaGetBufferDeviceAddress(context.device, instancesBuffer),
    };
    const VkAccelerationStructureGeometryInstancesDataKHR geometryInstancesData = {
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR,
        .arrayOfPointers = VK_FALSE,
        .data = instancesDataAddress,
    };
    const VkAccelerationStructureGeometryDataKHR geometryData = {
        .instances = geometryInstancesData,
    };
    const VkAccelerationStructureGeometryKHR geometry = {
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
        .geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR,
        .geometry = geometryData,
        .flags = VK_GEOMETRY_OPAQUE_BIT_KHR,
    };

    const VkAccelerationStructureBuildGeometryInfoKHR buildSizesGeometryInfo = {
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
        .type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,
        .flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
        .mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR,
        .geometryCount = 1,
        .pGeometries = &geometry,
    };
    VkAccelerationStructureBuildSizesInfoKHR buildSizesInfo = {
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR,
    };
    const uint32_t instanceCount = instances.size();
    vkGetAccelerationStructureBuildSizesKHR(lunaGetVkDevice(context.device),
                                            VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
                                            &buildSizesGeometryInfo,
                                            &instanceCount,
                                            &buildSizesInfo);

    const LunaBufferCreationInfo bufferCreationInfo = {
        .size = buildSizesInfo.accelerationStructureSize,
        .alignment = 256, // 256 is directly required by the spec
        .memoryType = LUNA_MEMORY_TYPE_DEVICE_LOCAL,
        .additionalUsageFlags = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR |
                                VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
    };
    if (!VulkanContext::CheckResult(lunaCreateBuffer(context.device, &bufferCreationInfo, &buffer)))
    {
        buffer = LUNA_NULL_HANDLE;
        lunaDestroyBuffer(instancesBuffer);
        instancesBuffer = LUNA_NULL_HANDLE;
        return;
    }

    const VkAccelerationStructureCreateInfoKHR createInfo = {
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR,
        .buffer = lunaGetVkBuffer(buffer),
        .offset = lunaGetBufferOffset(buffer),
        .size = lunaGetBufferSize(buffer),
        .type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,
    };
    if (!VulkanContext::CheckResult(vkCreateAccelerationStructureKHR(lunaGetVkDevice(context.device),
                                                                     &createInfo,
                                                                     nullptr,
                                                                     &accelerationStructure)))
    {
        accelerationStructure = VK_NULL_HANDLE;
        lunaDestroyBuffer(buffer);
        buffer = LUNA_NULL_HANDLE;
        lunaDestroyBuffer(instancesBuffer);
        instancesBuffer = LUNA_NULL_HANDLE;
        return;
    }

    const LunaBufferCreationInfo scratchBufferCreationInfo = {
        .size = buildSizesInfo.buildScratchSize,
        .alignment = context.physicalDeviceAccelerationStructureProperties
                             .minAccelerationStructureScratchOffsetAlignment,
        .memoryType = LUNA_MEMORY_TYPE_DEVICE_LOCAL,
        .additionalUsageFlags = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
    };
    if (!VulkanContext::CheckResult(lunaCreateBuffer(context.device, &scratchBufferCreationInfo, &scratchBuffer)))
    {
        scratchBuffer = LUNA_NULL_HANDLE;
        vkDestroyAccelerationStructureKHR(lunaGetVkDevice(context.device), accelerationStructure, nullptr);
        accelerationStructure = VK_NULL_HANDLE;
        lunaDestroyBuffer(buffer);
        buffer = LUNA_NULL_HANDLE;
        lunaDestroyBuffer(instancesBuffer);
        instancesBuffer = LUNA_NULL_HANDLE;
        return;
    }

    const VkDeviceOrHostAddressKHR scratchDataAddress = {
        .deviceAddress = lunaGetBufferDeviceAddress(context.device, scratchBuffer),
    };
    const VkAccelerationStructureBuildGeometryInfoKHR buildGeometryInfo = {
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
        .type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,
        .flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
        .mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR,
        .dstAccelerationStructure = accelerationStructure,
        .geometryCount = 1,
        .pGeometries = &geometry,
        .scratchData = scratchDataAddress,
    };
    const VkAccelerationStructureBuildRangeInfoKHR BUILD_RANGE_INFO = {
        .primitiveCount = instanceCount,
    };
    const VkAccelerationStructureBuildRangeInfoKHR *pBuildRangeInfo = &BUILD_RANGE_INFO;
    vkCmdBuildAccelerationStructuresKHR(lunaGetVkCommandBuffer(context.commandBuffer),
                                        1,
                                        &buildGeometryInfo,
                                        &pBuildRangeInfo);

    if (!VulkanContext::CheckResult(lunaEndAndSubmitCommandBuffer(context.device,
                                                                  context.commandBuffer,
                                                                  &context.submitInfo)))
    {
        lunaDestroyBuffer(scratchBuffer);
        scratchBuffer = LUNA_NULL_HANDLE;
        vkDestroyAccelerationStructureKHR(lunaGetVkDevice(context.device), accelerationStructure, nullptr);
        accelerationStructure = VK_NULL_HANDLE;
        lunaDestroyBuffer(buffer);
        buffer = LUNA_NULL_HANDLE;
        lunaDestroyBuffer(instancesBuffer);
        instancesBuffer = LUNA_NULL_HANDLE;
        return;
    }
}

AccelerationStructure::AccelerationStructure(const VulkanContext &context,
                                             const VkAccelerationStructureGeometryTrianglesDataKHR
                                                     &geometryTrianglesData,
                                             uint32_t triangleCount,
                                             const VkTransformMatrixKHR &transform,
                                             uint32_t firstIndex):
    context(context),
    instance(transform, firstIndex, 0xFF, 0, 0, 0)
{
    const VkDevice vkDevice = lunaGetVkDevice(context.device);

    const VkAccelerationStructureGeometryDataKHR geometryData = {
        .triangles = geometryTrianglesData,
    };
    const VkAccelerationStructureGeometryKHR geometry = {
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
        .geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR,
        .geometry = geometryData,
        .flags = VK_GEOMETRY_OPAQUE_BIT_KHR,
    };

    const VkAccelerationStructureBuildGeometryInfoKHR buildSizesGeometryInfo = {
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
        .type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
        .flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
        .mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR,
        .geometryCount = 1,
        .pGeometries = &geometry,
    };
    VkAccelerationStructureBuildSizesInfoKHR buildSizesInfo = {
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR,
    };
    vkGetAccelerationStructureBuildSizesKHR(vkDevice,
                                            VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
                                            &buildSizesGeometryInfo,
                                            &triangleCount,
                                            &buildSizesInfo);

    const LunaBufferCreationInfo bufferCreationInfo = {
        .size = buildSizesInfo.accelerationStructureSize,
        .alignment = 256, // 256 is directly required by the spec
        .memoryType = LUNA_MEMORY_TYPE_DEVICE_LOCAL,
        .additionalUsageFlags = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR |
                                VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
    };
    if (!VulkanContext::CheckResult(lunaCreateBuffer(context.device, &bufferCreationInfo, &buffer)))
    {
        buffer = LUNA_NULL_HANDLE;
        return;
    }

    const VkAccelerationStructureCreateInfoKHR createInfo = {
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR,
        .buffer = lunaGetVkBuffer(buffer),
        .offset = lunaGetBufferOffset(buffer),
        .size = lunaGetBufferSize(buffer),
        .type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
    };
    if (!VulkanContext::CheckResult(vkCreateAccelerationStructureKHR(vkDevice,
                                                                     &createInfo,
                                                                     nullptr,
                                                                     &accelerationStructure)))
    {
        lunaDestroyBuffer(buffer);
        buffer = LUNA_NULL_HANDLE;
        accelerationStructure = VK_NULL_HANDLE;
        return;
    }

    const LunaBufferCreationInfo scratchBufferCreationInfo = {
        .size = buildSizesInfo.buildScratchSize,
        .alignment = context.physicalDeviceAccelerationStructureProperties
                             .minAccelerationStructureScratchOffsetAlignment,
        .memoryType = LUNA_MEMORY_TYPE_DEVICE_LOCAL,
        .additionalUsageFlags = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
    };
    if (!VulkanContext::CheckResult(lunaCreateBuffer(context.device, &scratchBufferCreationInfo, &scratchBuffer)))
    {
        lunaDestroyBuffer(buffer);
        buffer = LUNA_NULL_HANDLE;
        vkDestroyAccelerationStructureKHR(vkDevice, accelerationStructure, nullptr);
        accelerationStructure = VK_NULL_HANDLE;
        scratchBuffer = LUNA_NULL_HANDLE;
        return;
    }

    const VkDeviceOrHostAddressKHR scratchDataAddress = {
        .deviceAddress = lunaGetBufferDeviceAddress(context.device, scratchBuffer),
    };
    const VkAccelerationStructureBuildGeometryInfoKHR buildGeometryInfo = {
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
        .type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
        .flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
        .mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR,
        .dstAccelerationStructure = accelerationStructure,
        .geometryCount = 1,
        .pGeometries = &geometry,
        .scratchData = scratchDataAddress,
    };
    const VkAccelerationStructureBuildRangeInfoKHR buildRangeInfo = {
        .primitiveCount = triangleCount,
    };
    const VkAccelerationStructureBuildRangeInfoKHR *pBuildRangeInfo = &buildRangeInfo;
    vkCmdBuildAccelerationStructuresKHR(lunaGetVkCommandBuffer(context.commandBuffer),
                                        1,
                                        &buildGeometryInfo,
                                        &pBuildRangeInfo);

    const VkAccelerationStructureDeviceAddressInfoKHR deviceAddressInfo = {
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR,
        .accelerationStructure = accelerationStructure,
    };
    instance.accelerationStructureReference = vkGetAccelerationStructureDeviceAddressKHR(vkDevice, &deviceAddressInfo);
}

void AccelerationStructure::WriteToDescriptorSet(const VkDescriptorSet &descriptorSet,
                                                 const uint32_t bindingIndex) const
{
    const VkWriteDescriptorSetAccelerationStructureKHR accelerationStructureWriteInfo = {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR,
        .accelerationStructureCount = 1,
        .pAccelerationStructures = &accelerationStructure,
    };
    const VkWriteDescriptorSet write = {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .pNext = &accelerationStructureWriteInfo,
        .dstSet = descriptorSet,
        .dstBinding = bindingIndex,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,
    };
    vkUpdateDescriptorSets(lunaGetVkDevice(context.get().device), 1, &write, 0, nullptr);
}

bool AccelerationStructure::IsValid() const
{
    return (instance.accelerationStructureReference != 0 || instancesBuffer != LUNA_NULL_HANDLE) &&
           buffer != LUNA_NULL_HANDLE &&
           accelerationStructure != VK_NULL_HANDLE &&
           scratchBuffer != LUNA_NULL_HANDLE;
}

const VkAccelerationStructureInstanceKHR &AccelerationStructure::InstanceData() const
{
    return instance;
}
