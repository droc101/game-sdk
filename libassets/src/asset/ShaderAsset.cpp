//
// Created by droc101 on 7/23/25.
//

#include <cstddef>
#include <cstdint>
#include <fstream>
#include <ios>
#include <libassets/asset/Asset.h>
#include <libassets/asset/ShaderAsset.h>
#include <libassets/util/AssetContainer.h>
#include <libassets/util/DataReader.h>
#include <libassets/util/DataWriter.h>
#include <libassets/util/Error.h>
#include <libassets/util/ShaderCompiler.h>
#include <shaderc/shaderc.h>
#include <sstream>
#include <vector>

Asset::AssetType ShaderAsset::GetAssetType() const
{
    return AssetType::ASSET_TYPE_SHADER;
}

uint8_t ShaderAsset::GetAssetTypeVersion() const
{
    return SHADER_ASSET_VERSION;
}

Error::ErrorCode ShaderAsset::LoadFromBuffer(DataReader &reader)
{
    kind = static_cast<ShaderKind>(reader.Read<uint8_t>());
    const size_t glslLength = reader.Read<size_t>();
    glsl = "";
    reader.ReadString(glsl, glslLength);
    // Following entries are present in the binary format but are not used for editing and therefore are not read
    // size_t spirvSize;
    // uint32_t[spirvSize] spirvData;
    return Error::ErrorCode::OK;
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
    writer.Write<size_t>(glsl.length() + 1);
    writer.WriteBuffer(glsl.c_str(), glsl.length());
    writer.Write<uint8_t>(0); // null byte
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
    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    if (!file)
    {
        return Error::ErrorCode::CANT_OPEN_FILE;
    }
    file.seekg(0, std::ios::beg);
    std::stringstream buffer;
    buffer << file.rdbuf();
    glsl = buffer.str();
    file.close();
    return Error::ErrorCode::OK;
}

Error::ErrorCode ShaderAsset::Export(const std::string &filePath) const
{
    std::ofstream file(filePath);
    if (!file)
    {
        return Error::ErrorCode::CANT_OPEN_FILE;
    }
    file.write(glsl.c_str(), static_cast<std::streamsize>(glsl.length()));
    file.close();
    return Error::ErrorCode::OK;
}

Error::ErrorCode ShaderAsset::SaveToAssetEx(const std::string &filePath,
                                            const bool enableOptimization,
                                            std::string *errorLog,
                                            const std::string &shaderFilename) const
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
    return AssetContainer::SaveToFile(filePath.c_str(),
                                      data,
                                      GetAssetType(),
                                      GetAssetTypeVersion(),
                                      AssetContainer::BEST_COMPRESSION);
}

std::string &ShaderAsset::GetGLSL()
{
    return glsl;
}
