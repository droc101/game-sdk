//
// Created by droc101 on 7/22/25.
//

#pragma once

#include <cstdint>
#include <filesystem>
#include <libassets/util/Error.h>
#include <shaderc/shaderc.hpp>
#include <string>
#include <vector>

class ShaderCompiler
{
    public:
        ShaderCompiler() = delete;

        ShaderCompiler(std::string glslSource,
                       shaderc_shader_kind shaderKind,
                       std::string shaderName,
                       bool optimize);

        ShaderCompiler(const std::filesystem::path &path, shaderc_shader_kind shaderKind, bool optimize);

        [[nodiscard]] Error::ErrorCode Compile(std::vector<uint32_t> &outputSpirv);

        [[nodiscard]] const std::string &GetErrorMessage() const;

    private:
        shaderc::CompileOptions options{};

        shaderc_shader_kind shaderKind;

        std::string glslSource;

        std::string shaderName;

        std::string errorMessage;
};
