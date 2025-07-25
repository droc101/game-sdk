//
// Created by droc101 on 7/22/25.
//

#ifndef SHADERCOMPILER_H
#define SHADERCOMPILER_H

#include <glslang/Public/ShaderLang.h>
#include <vulkan/vulkan_core.h>
#include <libassets/util/Error.h>

class ShaderCompiler
{
    public:
        ShaderCompiler() = delete;
        ShaderCompiler(const std::string &glslSource, VkShaderStageFlagBits shaderStage);
        ShaderCompiler(const std::string &glslSource, VkShaderStageFlagBits shaderStage, glslang::EShTargetClientVersion targetVulkanVersion);

        [[nodiscard]] Error::ErrorCode Compile(std::vector<uint32_t> &outputSpirv) const;

        void SetTargetVersions(glslang::EShTargetClientVersion targetVulkanVersion, glslang::EShTargetLanguageVersion targetSpirvVersion);

    private:
        static EShLanguage FindShaderLanguage(VkShaderStageFlagBits stage);
        static TBuiltInResource GetResources();

        glslang::EShTargetClientVersion targetVulkanVersion = glslang::EShTargetVulkan_1_0;
        glslang::EShTargetLanguageVersion targetSpirvVersion = glslang::EShTargetSpv_1_0;
        VkShaderStageFlagBits shaderStage = VK_SHADER_STAGE_VERTEX_BIT;
        std::string glslSource{};
};

#endif //SHADERCOMPILER_H
