//
// Created by droc101 on 7/22/25.
//

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <glslang/Public/ResourceLimits.h>
#include <glslang/Public/ShaderLang.h>
#include <libassets/util/Error.h>
#include <libassets/util/FileIo.h>
#include <libassets/util/Logger.h>
#include <libassets/util/ShaderCompiler.h>
#include <list>
#include <SPIRV/GlslangToSpv.h>
#include <string>
#include <utility>
#include <vector>

ShaderCompiler::SDKIncluder &ShaderCompiler::SDKIncluder::Get()
{
    thread_local SDKIncluder includer{};
    return includer;
}

ShaderCompiler::SDKIncluder::IncludeResult *ShaderCompiler::SDKIncluder::includeLocal(const char *requestedSource,
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
    includeResults.emplace_back(requestedSourcePath.string(), strdup(glslString.c_str()), glslString.length(), nullptr);
    return &includeResults.back();
}

void ShaderCompiler::SDKIncluder::releaseInclude(IncludeResult *data)
{
    for (std::list<IncludeResult>::iterator iterator = includeResults.begin(); iterator != includeResults.end();
         ++iterator)
    {
        if (&*iterator == data)
        {
            free(const_cast<char *>(iterator->headerData));
            includeResults.erase(iterator);
            break;
        }
    }
}

ShaderCompiler::ShaderCompiler(std::string glslSource,
                               const EShLanguage shaderType,
                               std::string shaderName,
                               const bool optimize,
                               const bool debugInfo):
    shaderType(shaderType),
    glslSource(std::move(glslSource)),
    shaderPath(std::move(shaderName)),
    optimize(optimize),
    debugInfo(debugInfo)
{}

ShaderCompiler::ShaderCompiler(const std::filesystem::path &path,
                               const EShLanguage shaderType,
                               const bool optimize,
                               const bool debugInfo):
    ShaderCompiler("", shaderType, path.string(), optimize, debugInfo)
{
    FileIo::ReadFileToString(path.string(), this->glslSource);
}

Error::ErrorCode ShaderCompiler::Compile(std::vector<uint32_t> &outputSpirv)
{
    if (!outputSpirv.empty())
    {
        compileLog = "";
        return Error::ErrorCode::INVALID_ARGUMENT;
    }
    if (!glslang::InitializeProcess())
    {
        compileLog = "Failed to initialize glslang!";
        return Error::ErrorCode::UNKNOWN;
    }

    glslang::TShader shader(shaderType);
    const char *glsl = glslSource.c_str();
    const int glslLength = static_cast<int>(glslSource.length());
    const char *glslPath = shaderPath.c_str();
    shader.setStringsWithLengthsAndNames(&glsl, &glslLength, &glslPath, 1);
    shader.setEnvInput(glslang::EShSourceGlsl, shaderType, glslang::EShClientVulkan, 100);
    shader.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_2);
    shader.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_5);
    shader.setEnhancedMsgs();
    const std::string preamble = "#extension GL_GOOGLE_include_directive : require\n" + GetShaderTypeMacro() + "\n";
    shader.setPreamble(preamble.c_str());

    constexpr EShMessages MESSAGES = static_cast<EShMessages>(EShMsgSpvRules | EShMsgVulkanRules);

    if (!shader.parse(GetDefaultResources(), 100, ECoreProfile, false, false, MESSAGES, SDKIncluder::Get()))
    {
        compileLog = shader.getInfoLog();
        Logger::Error("GLSL Parsing Failed:\n {}", shader.getInfoLog());
        return Error::ErrorCode::SHADER_PARSE_ERROR;
    }

    glslang::TProgram program;
    program.addShader(&shader);

    if (!program.link(MESSAGES))
    {
        compileLog = program.getInfoLog();
        Logger::Error("GLSL Linking Failed:\n {}", program.getInfoLog());
        return Error::ErrorCode::SHADER_LINK_ERROR;
    }

    glslang::SpvOptions options = {
        .generateDebugInfo = debugInfo,
        .stripDebugInfo = !debugInfo,
        .disableOptimizer = !optimize,
        .validate = true,
        .emitNonSemanticShaderDebugInfo = debugInfo,
        .emitNonSemanticShaderDebugSource = debugInfo,
    };

    glslang::GlslangToSpv(*program.getIntermediate(shaderType), outputSpirv, &options);
    glslang::FinalizeProcess();

    return Error::ErrorCode::OK;
}

const std::string &ShaderCompiler::GetErrorMessage() const
{
    return compileLog;
}

std::string ShaderCompiler::GetShaderTypeMacro() const noexcept
{
    switch (shaderType)
    {
        case EShLangVertex:
            return "#define SHADER_TYPE_VERTEX";
        case EShLangTessControl:
            return "#define SHADER_TYPE_TESSELLATION_CONTROL";
        case EShLangTessEvaluation:
            return "#define SHADER_TYPE_TESSELLATION_EVALUATION";
        case EShLangGeometry:
            return "#define SHADER_TYPE_GEOMETRY";
        case EShLangFragment:
            return "#define SHADER_TYPE_FRAGMENT";
        case EShLangCompute:
            return "#define SHADER_TYPE_COMPUTE";
        case EShLangRayGen:
            return "#define SHADER_TYPE_RAYGEN";
        case EShLangIntersect:
            return "#define SHADER_TYPE_INTERSECTION";
        case EShLangAnyHit:
            return "#define SHADER_TYPE_ANY_HIT";
        case EShLangClosestHit:
            return "#define SHADER_TYPE_CLOSEST_HIT";
        case EShLangMiss:
            return "#define SHADER_TYPE_MISS";
        case EShLangCallable:
            return "#define SHADER_TYPE_CALLABLE";
        case EShLangTask:
            return "#define SHADER_TYPE_TASK";
        case EShLangMesh:
            return "#define SHADER_TYPE_MESH";
        default:
            return "";
    }
}
