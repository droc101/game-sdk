//
// Created by droc101 on 7/22/25.
//

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <libassets/util/Error.h>
#include <libassets/util/Logger.h>
#include <libassets/util/ShaderCompiler.h>
#include <shaderc/shaderc.hpp>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

ShaderCompiler::ShaderCompiler(std::string glslSource,
                               const shaderc_shader_kind shaderKind,
                               std::string shaderName,
                               const bool optimize):
    shaderKind(shaderKind),
    glslSource(std::move(glslSource)),
    shaderName(std::move(shaderName))
{
    options.SetSourceLanguage(shaderc_source_language_glsl);
    options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_4);
    if (optimize)
    {
        options.SetOptimizationLevel(shaderc_optimization_level_performance);
    } else
    {
        options.SetGenerateDebugInfo();
    }
}

ShaderCompiler::ShaderCompiler(const std::filesystem::path &path,
                               const shaderc_shader_kind shaderKind,
                               const bool optimize):
    ShaderCompiler("", shaderKind, path.filename().string(), optimize)
{
    std::ifstream glslFile(path);
    std::stringstream glsl;
    glsl << glslFile.rdbuf();
    glslFile.close();

    this->glslSource = glsl.str();
}

Error::ErrorCode ShaderCompiler::Compile(std::vector<uint32_t> &outputSpirv)
{
    if (!outputSpirv.empty())
    {
        errorMessage = "";
        return Error::ErrorCode::INVALID_ARGUMENT;
    }

    const shaderc::Compiler compiler{};
    const shaderc::SpvCompilationResult result = compiler.CompileGlslToSpv(glslSource, shaderc_vertex_shader, "");
    if (result.GetCompilationStatus() != shaderc_compilation_status_success)
    {
        errorMessage = result.GetErrorMessage();
        return result.GetCompilationStatus() == shaderc_compilation_status_compilation_error
                       ? Error::ErrorCode::SHADER_COMPILE_ERROR
                       : Error::ErrorCode::UNKNOWN;
    }

    outputSpirv.insert(outputSpirv.begin(), result.begin(), result.end());

    return Error::ErrorCode::OK;
}

const std::string &ShaderCompiler::GetErrorMessage() const
{
    return errorMessage;
}
