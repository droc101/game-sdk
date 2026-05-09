//
// Created by droc101 on 7/22/25.
//

#pragma once

#include <cstdint>
#include <filesystem>
#include <glslang/Include/ResourceLimits.h>
#include <glslang/Public/ShaderLang.h>
#include <libassets/util/Error.h>
#include <string>
#include <vector>

class ShaderCompiler
{
    public:
        ShaderCompiler() = delete;

        ShaderCompiler(const std::string &glslSource, EShLanguage shaderType);

        ShaderCompiler(const std::string &glslSource,
                       EShLanguage shaderType,
                       glslang::EShTargetClientVersion targetVulkanVersion);

        ShaderCompiler(const std::filesystem::path &path, EShLanguage shaderType);

        [[nodiscard]] Error::ErrorCode Compile(std::vector<uint32_t> &outputSpirv) const;

        void SetTargetVersions(glslang::EShTargetClientVersion targetVulkanVersion,
                               glslang::EShTargetLanguageVersion targetSpirvVersion);

    private:
        static TBuiltInResource GetResources();

        glslang::EShTargetClientVersion targetVulkanVersion = glslang::EShTargetVulkan_1_0;

        glslang::EShTargetLanguageVersion targetSpirvVersion = glslang::EShTargetSpv_1_0;

        EShLanguage shaderType = EShLangVertex;

        std::string glslSource{};
};
