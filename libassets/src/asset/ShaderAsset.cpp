//
// Created by droc101 on 7/23/25.
//

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <libassets/asset/Asset.h>
#include <libassets/asset/ShaderAsset.h>
#include <libassets/util/AssetContainer.h>
#include <libassets/util/DataWriter.h>
#include <libassets/util/Error.h>
#include <libassets/util/FileIo.h>
#include <libassets/util/ShaderCompiler.h>
#include <shaderc/shaderc.h>
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
    kind = ShaderKind::SHADER_KIND_FRAGMENT;
    glsl = "";
}

Error::ErrorCode ShaderAsset::SaveToBuffer(DataWriter &writer) const
{
    return SaveToBufferEx(writer, false);
}

Error::ErrorCode ShaderAsset::SaveToBufferEx(DataWriter &writer,
                                             const bool enableOptimization,
                                             std::string *errorLog,
                                             const std::string &shaderFilename) const
{
    writer.Write<uint8_t>(static_cast<uint8_t>(kind));
    std::vector<uint32_t> spirv;
    const shaderc_shader_kind shaderKind = kind == ShaderKind::SHADER_KIND_VERTEX ? shaderc_vertex_shader
                                                                                  : shaderc_fragment_shader;
    ShaderCompiler compiler = ShaderCompiler(glsl, shaderKind, shaderFilename, enableOptimization);
    const Error::ErrorCode error = compiler.Compile(spirv);
    if (error != Error::ErrorCode::OK)
    {
        if (errorLog != nullptr)
        {
            *errorLog = compiler.GetErrorMessage();
        }
        return error;
    }

    writer.Write<size_t>(spirv.size());
    writer.WriteBuffer<uint32_t>(spirv);
    return Error::ErrorCode::OK;
}

Error::ErrorCode ShaderAsset::Import(const std::string &filePath)
{
    Reset();
    return FileIo::ReadFileToString(filePath, glsl);
}

Error::ErrorCode ShaderAsset::SaveToAssetEx(const std::string &filePath,
                                            const bool enableOptimization,
                                            std::string *errorLog,
                                            const std::string &shaderFilename,
                                            const bool dumpSpvBinary) const
{
    DataWriter writer{};
    const Error::ErrorCode writeError = SaveToBufferEx(writer, enableOptimization, errorLog, shaderFilename);
    if (writeError != Error::ErrorCode::OK)
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
        const Error::ErrorCode e = FileIo::WriteBufferToFile(filePath + ".spv", spv);
        if (e != Error::ErrorCode::OK)
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
