//
// Created by droc101 on 7/22/25.
//

#pragma once

#include <cstdint>
#include <glslang/Include/ResourceLimits.h>
#include <glslang/Public/ShaderLang.h>
#include <libassets/util/Error.h>
#include <string>
#include <vector>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_enums.hpp>

class ShaderCompiler
{
    public:
        ShaderCompiler() = delete;

        ShaderCompiler(const std::string &glslSource, vk::ShaderStageFlagBits shaderStage);

        ShaderCompiler(const std::string &glslSource,
                       vk::ShaderStageFlagBits shaderStage,
                       glslang::EShTargetClientVersion targetVulkanVersion);

        [[nodiscard]] Error::ErrorCode Compile(std::vector<uint32_t> &outputSpirv) const;

        void SetTargetVersions(glslang::EShTargetClientVersion targetVulkanVersion,
                               glslang::EShTargetLanguageVersion targetSpirvVersion);

    private:
        static EShLanguage FindShaderLanguage(vk::ShaderStageFlagBits stage);

        static TBuiltInResource GetResources();

        glslang::EShTargetClientVersion targetVulkanVersion = glslang::EShTargetVulkan_1_0;

        glslang::EShTargetLanguageVersion targetSpirvVersion = glslang::EShTargetSpv_1_0;

        vk::ShaderStageFlagBits shaderStage = vk::ShaderStageFlagBits::eVertex;

        std::string glslSource{};
};
