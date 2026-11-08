//
// Created by droc101 on 7/23/25.
//

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <glslang/Public/ShaderLang.h>
#include <iterator>
#include <libassets/asset/Asset.h>
#include <libassets/asset/ShaderAsset.h>
#include <libassets/util/AssetContainer.h>
#include <libassets/util/DataWriter.h>
#include <libassets/util/Error.h>
#include <libassets/util/FileIo.h>
#include <libassets/util/ShaderCompiler.h>
#include <vector>

ShaderAsset::ShaderAsset()
{
    Reset();
}

Asset::AssetType ShaderAsset::GetAssetType() const
{
    return AssetType::ASSET_TYPE_SHADER;
}

uint8_t ShaderAsset::GetAssetTypeVersion() const
{
    return SHADER_ASSET_VERSION;
}

void ShaderAsset::Reset()
{
    type = ShaderType::SHADER_KIND_FRAGMENT;
    glsl = "";
}

const Error &ShaderAsset::SaveToBuffer(DataWriter &writer) const
{
    return SaveToBufferEx(writer, false, false);
}

const Error &ShaderAsset::SaveToBufferEx(DataWriter &writer,
                                         const bool enableOptimization,
                                         const bool debugInfo,
                                         std::string *errorLog,
                                         const std::string &shaderFilename) const
{
    writer.Write<uint8_t>(static_cast<uint8_t>(type));
    std::vector<uint32_t> spirv;
    EShLanguage shaderType{};
    switch (type)
    {
        case ShaderType::SHADER_KIND_FRAGMENT:
            shaderType = EShLangFragment;
            break;
        case ShaderType::SHADER_KIND_VERTEX:
            shaderType = EShLangVertex;
            break;
        case ShaderType::SHADER_KIND_COMPUTE:
            shaderType = EShLangCompute;
            break;
        case ShaderType::SHADER_KIND_GEOMETRY:
            shaderType = EShLangGeometry;
            break;
    }
    ShaderCompiler compiler = ShaderCompiler(glsl, shaderType, shaderFilename, enableOptimization, debugInfo);
    const Error &error = compiler.Compile(spirv);
    if (error.Failed())
    {
        if (errorLog != nullptr)
        {
            *errorLog = compiler.GetErrorMessage();
        }
        return error;
    }

    writer.Write<size_t>(spirv.size());
    writer.WriteBuffer<uint32_t>(spirv);
    return Errors::OK;
}

const Error &ShaderAsset::Import(const std::string &filePath)
{
    Reset();
    return FileIo::ReadFileToString(filePath, glsl);
}

const Error &ShaderAsset::SaveToAssetEx(const std::string &filePath,
                                        const bool enableOptimization,
                                        const bool debugInfo,
                                        std::string *errorLog,
                                        const std::string &shaderFilename,
                                        const bool dumpSpvBinary) const
{
    DataWriter writer{};
    const Error &writeError = SaveToBufferEx(writer, enableOptimization, debugInfo, errorLog, shaderFilename);
    if (writeError.Failed())
    {
        return writeError;
    }
    std::vector<uint8_t> data{};
    data.reserve(writer.GetBufferSize());
    writer.CopyToVector(data);

    if (dumpSpvBinary)
    {
        std::vector<uint8_t> spv{};
        spv.reserve(data.size() - sizeof(size_t) - sizeof(uint8_t));
        std::copy(data.begin() + sizeof(size_t) + sizeof(uint8_t), data.end(), std::back_inserter(spv));
        const Error &e = FileIo::WriteBufferToFile(filePath + ".spv", spv);
        if (e.Failed())
        {
            return e;
        }
    }

    return AssetContainer::SaveToFile(filePath,
                                      data,
                                      GetAssetType(),
                                      GetAssetTypeVersion(),
                                      AssetContainer::BEST_COMPRESSION);
}
