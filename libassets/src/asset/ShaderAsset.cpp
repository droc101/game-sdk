//
// Created by droc101 on 7/23/25.
//

#include <cstddef>
#include <cstdint>
#include <fstream>
#include <glslang/Public/ShaderLang.h>
#include <ios>
#include <libassets/asset/ShaderAsset.h>
#include <libassets/util/Asset.h>
#include <libassets/util/AssetReader.h>
#include <libassets/util/DataWriter.h>
#include <libassets/util/Error.h>
#include <libassets/util/ShaderCompiler.h>
#include <sstream>
#include <vector>
#include <vulkan/vulkan_enums.hpp>

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
    shader.platform = static_cast<ShaderPlatform>(asset.reader.Read<uint8_t>());
    shader.type = static_cast<ShaderType>(asset.reader.Read<uint8_t>());
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

Error::ErrorCode ShaderAsset::SaveToBuffer(std::vector<uint8_t> &buffer) const
{
    DataWriter writer{};
    writer.Write<uint8_t>(static_cast<uint8_t>(platform));
    writer.Write<uint8_t>(static_cast<uint8_t>(type));
    writer.Write<size_t>(glsl.length() + 1);
    writer.WriteBuffer(glsl.c_str(), glsl.length());
    writer.Write<uint8_t>(0); // null byte
    if (platform == ShaderPlatform::PLATFORM_VULKAN)
    {
        std::vector<uint32_t> spirv;
        const vk::ShaderStageFlagBits stage = type == ShaderType::SHADER_TYPE_VERT
                                                  ? vk::ShaderStageFlagBits::eVertex
                                                  : vk::ShaderStageFlagBits::eFragment;
        ShaderCompiler compiler = ShaderCompiler(glsl, stage);
        compiler.SetTargetVersions(glslang::EShTargetClientVersion::EShTargetVulkan_1_2,
                                   glslang::EShTargetLanguageVersion::EShTargetSpv_1_0);
        const Error::ErrorCode error = compiler.Compile(spirv);
        if (error != Error::ErrorCode::OK)
        {
            return error;
        }

        writer.Write<size_t>(spirv.size());
        writer.WriteBuffer<uint32_t>(spirv);
    } else
    {
        writer.Write<size_t>(0); // zero spirv for opengl
    }
    writer.CopyToVector(buffer);
    return Error::ErrorCode::OK;
}

Error::ErrorCode ShaderAsset::SaveAsAsset(const char *assetPath) const
{
    std::vector<uint8_t> buffer;
    const Error::ErrorCode e = SaveToBuffer(buffer);
    if (e != Error::ErrorCode::OK)
    {
        return e;
    }
    return AssetReader::SaveToFile(assetPath, buffer, Asset::AssetType::ASSET_TYPE_SHADER, SHADER_ASSET_VERSION);
}

std::string &ShaderAsset::GetGLSL()
{
    return glsl;
}
