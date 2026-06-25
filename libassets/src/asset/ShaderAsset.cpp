//
// Created by droc101 on 7/23/25.
//

#include <cstddef>
#include <cstdint>
#include <fstream>
#include <ios>
#include <libassets/asset/ShaderAsset.h>
#include <libassets/type/Asset.h>
#include <libassets/util/AssetReader.h>
#include <libassets/util/DataWriter.h>
#include <libassets/util/Error.h>
#include <libassets/util/ShaderCompiler.h>
#include <sstream>
#include <vector>

Error::ErrorCode ShaderAsset::CreateFromAsset(const char *assetPath, ShaderAsset &shader)
{
    Asset asset;
    const Error::ErrorCode error = AssetReader::LoadFromFile(assetPath, asset);
    if (error != Error::ErrorCode::OK)
    {
        return error;
    }
    if (asset.type != Asset::AssetType::ASSET_TYPE_SHADER)
    {
        return Error::ErrorCode::INCORRECT_FORMAT;
    }
    if (asset.typeVersion != SHADER_ASSET_VERSION)
    {
        return Error::ErrorCode::INCORRECT_VERSION;
    }
    shader = ShaderAsset();
    shader.kind = static_cast<ShaderKind>(asset.reader.Read<uint8_t>());
    const size_t glslLength = asset.reader.Read<size_t>();
    shader.glsl = "";
    asset.reader.ReadString(shader.glsl, glslLength);
    // Following entries are present in the binary format but are not used for editing and therefore are not read
    // size_t spirvSize;
    // uint32_t[spirvSize] spirvData;
    return Error::ErrorCode::OK;
}

Error::ErrorCode ShaderAsset::CreateFromGlsl(const char *glslPath, ShaderAsset &shader)
{
    std::ifstream file(glslPath, std::ios::binary | std::ios::ate);
    if (!file)
    {
        return Error::ErrorCode::CANT_OPEN_FILE;
    }
    file.seekg(0, std::ios::beg);
    shader = ShaderAsset();
    std::stringstream buffer;
    buffer << file.rdbuf();
    shader.glsl = buffer.str();
    file.close();
    return Error::ErrorCode::OK;
}

Error::ErrorCode ShaderAsset::SaveAsGlsl(const char *glslPath) const
{
    std::ofstream file(glslPath);
    if (!file)
    {
        return Error::ErrorCode::CANT_OPEN_FILE;
    }
    file.write(glsl.c_str(), static_cast<std::streamsize>(glsl.length()));
    file.close();
    return Error::ErrorCode::OK;
}

Error::ErrorCode ShaderAsset::SaveToBuffer(const char *assetPath,
                                           const bool enableOptimization,
                                           std::vector<uint8_t> &buffer,
                                           std::string *errorLog) const
{
    DataWriter writer{};
    writer.Write<uint8_t>(static_cast<uint8_t>(kind));
    writer.Write<size_t>(glsl.length() + 1);
    writer.WriteBuffer(glsl.c_str(), glsl.length());
    writer.Write<uint8_t>(0); // null byte
    std::vector<uint32_t> spirv;
    const shaderc_shader_kind shaderKind = kind == ShaderKind::SHADER_KIND_VERTEX ? shaderc_vertex_shader
                                                                                  : shaderc_fragment_shader;
    ShaderCompiler compiler = ShaderCompiler(glsl, shaderKind, assetPath, enableOptimization);
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
    writer.CopyToVector(buffer);
    return Error::ErrorCode::OK;
}

Error::ErrorCode ShaderAsset::SaveAsAsset(const char *assetPath,
                                          const bool enableOptimization,
                                          std::string *errorLog) const
{
    std::vector<uint8_t> buffer;
    const Error::ErrorCode e = SaveToBuffer(assetPath, enableOptimization, buffer, errorLog);
    if (e != Error::ErrorCode::OK)
    {
        return e;
    }
    return AssetReader::SaveToFile(assetPath,
                                   buffer,
                                   Asset::AssetType::ASSET_TYPE_SHADER,
                                   SHADER_ASSET_VERSION,
                                   AssetReader::BEST_COMPRESSION);
}

std::string &ShaderAsset::GetGLSL()
{
    return glsl;
}
