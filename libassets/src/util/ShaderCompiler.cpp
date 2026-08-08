//
// Created by droc101 on 7/22/25.
//

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <libassets/util/Error.h>
#include <libassets/util/FileIo.h>
#include <libassets/util/Logger.h>
#include <libassets/util/ShaderCompiler.h>
#include <list>
#include <memory>
#include <shaderc/env.h>
#include <shaderc/shaderc.h>
#include <shaderc/shaderc.hpp>
#include <shaderc/status.h>
#include <string>
#include <utility>
#include <vector>

shaderc_include_result *ShaderCompiler::SDKIncluder::GetInclude(const char *requestedSource,
                                                                shaderc_include_type /*type*/,
                                                                const char *requestingSource,
                                                                size_t /*includeDepth*/)
{
    std::filesystem::path requestingSourcePath{requestingSource};
    const std::filesystem::path requestedSourcePath = requestingSourcePath.remove_filename().append(requestedSource);
    std::string glslString{};
    const Error::ErrorCode readError = FileIo::ReadFileToString(requestedSourcePath.string(), glslString);
    if (readError != Error::ErrorCode::OK)
    {
        Logger::Error("Failed to read include file \"{}\": {}", requestedSource, readError);
        glslString = "";
    }
    includeResults.emplace_back(strdup(requestedSourcePath.string().c_str()),
                                requestedSourcePath.string().length(),
                                strdup(glslString.c_str()),
                                glslString.length(),
                                nullptr);
    return &includeResults.back();
}

void ShaderCompiler::SDKIncluder::ReleaseInclude(shaderc_include_result *data)
{
    for (std::list<shaderc_include_result>::iterator iterator = includeResults.begin();
         iterator != includeResults.end();
         ++iterator)
    {
        if (&*iterator == data)
        {
            free((void *)(iterator->source_name));
            free((void *)(iterator->content));
            includeResults.erase(iterator);
            break;
        }
    }
}

ShaderCompiler::ShaderCompiler(std::string glslSource,
                               const shaderc_shader_kind shaderKind,
                               std::string shaderName,
                               const bool optimize):
    shaderKind(shaderKind),
    glslSource(std::move(glslSource)),
    shaderPath(std::move(shaderName))
{
    options.SetSourceLanguage(shaderc_source_language_glsl);
    options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_2);
    if (optimize)
    {
        options.SetOptimizationLevel(shaderc_optimization_level_performance);
    } else
    {
        options.SetGenerateDebugInfo();
    }
    options.SetIncluder(std::make_unique<SDKIncluder>());
}

ShaderCompiler::ShaderCompiler(const std::filesystem::path &path,
                               const shaderc_shader_kind shaderKind,
                               const bool optimize):
    ShaderCompiler("", shaderKind, path.string(), optimize)
{
    FileIo::ReadFileToString(path.string(), this->glslSource);
}

Error::ErrorCode ShaderCompiler::Compile(std::vector<uint32_t> &outputSpirv)
{
    if (!outputSpirv.empty())
    {
        errorMessage = "";
        return Error::ErrorCode::INVALID_ARGUMENT;
    }

    const shaderc::Compiler compiler{};
    const shaderc::SpvCompilationResult result = compiler.CompileGlslToSpv(glslSource,
                                                                           shaderKind,
                                                                           shaderPath.c_str(),
                                                                           "main",
                                                                           options);
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
