//
// Created by droc101 on 7/22/25.
//

#include "OpenGLImGuiTextureAssetCache.h"
#include <cstdint>
#include <filesystem>
#include <imgui.h>
#include <libassets/asset/TextureAsset.h>
#include <libassets/util/Error.h>
#include <ranges>
#include <SDL3/SDL_endian.h>
#include <string>
#include <vector>
#include "Options.h"

OpenGLImGuiTextureAssetCache::OpenGLImGuiTextureAssetCache(): ImGuiTextureAssetCache() {}

OpenGLImGuiTextureAssetCache::~OpenGLImGuiTextureAssetCache()
{
    for (const ImTextureID &textureId: textureBuffers | std::views::values)
    {
        const GLuint &tex = textureId;
        glDeleteTextures(1, &tex);
    }
    textureBuffers.clear();
}

void OpenGLImGuiTextureAssetCache::InitMissingTexture()
{
    TextureAsset texture;
    TextureAsset::CreateMissingTexture(texture);
    this->missingTexture = static_cast<ImTextureID>(CreateTexture(texture));
}


GLuint OpenGLImGuiTextureAssetCache::CreateTexture(const TextureAsset &textureAsset)
{
    std::vector<uint32_t> pixels;
    textureAsset.GetPixelsRGBA(pixels);
    for (uint32_t &pixel: pixels)
    {
        pixel = SDL_Swap32BE(pixel);
    }

    GLuint glTexture = 0;
    glGenTextures(1, &glTexture);
    glBindTexture(GL_TEXTURE_2D, glTexture);
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_RGBA,
                 static_cast<GLsizei>(textureAsset.GetWidth()),
                 static_cast<GLsizei>(textureAsset.GetHeight()),
                 0,
                 GL_RGBA,
                 GL_UNSIGNED_BYTE,
                 pixels.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    return glTexture;
}

Error::ErrorCode OpenGLImGuiTextureAssetCache::GetTextureID(const std::string &relPath, ImTextureID &outTexture)
{
    if (textureBuffers.contains(relPath))
    {
        outTexture = textureBuffers.at(std::string(relPath));
        return Error::ErrorCode::OK;
    }
    const std::string texturePath = Options::GetAssetsPath() + "/" + relPath;
    if (!std::filesystem::exists(texturePath))
    {
        outTexture = missingTexture;
        return Error::ErrorCode::OK;
    }
    TextureAsset asset;
    const Error::ErrorCode error = TextureAsset::CreateFromAsset(texturePath.c_str(), asset);
    if (error != Error::ErrorCode::OK)
    {
        return error;
    }
    const GLuint glTex = CreateTexture(asset);
    textureBuffers.insert({relPath, glTex});
    outTexture = glTex;
    return Error::ErrorCode::OK;
}

Error::ErrorCode OpenGLImGuiTextureAssetCache::GetTextureSize(const std::string &relPath, ImVec2 &outSize)
{
    ImTextureID texture = 0;
    const Error::ErrorCode error = GetTextureID(relPath, texture);
    if (error != Error::ErrorCode::OK)
    {
        return error;
    }
    glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(texture));
    int w = 0;
    int h = 0;
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &w);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &h);
    outSize = {static_cast<float>(w), static_cast<float>(h)};
    return Error::ErrorCode::OK;
}

Error::ErrorCode OpenGLImGuiTextureAssetCache::GetTextureGLuint(const std::string &relPath, GLuint &outTexture)
{
    ImTextureID texture = 0;
    const Error::ErrorCode error = GetTextureID(relPath, texture);
    if (error != Error::ErrorCode::OK)
    {
        return error;
    }
    outTexture = static_cast<GLuint>(texture);
    return Error::ErrorCode::OK;
}

Error::ErrorCode OpenGLImGuiTextureAssetCache::LoadTexture(const std::string &relPath)
{
    ImTextureID unused = 0;
    return GetTextureID(relPath, unused);
}

Error::ErrorCode OpenGLImGuiTextureAssetCache::RegisterPng(const std::string &pngPath, const std::string &name)
{
    TextureAsset tex;
    const Error::ErrorCode e = TextureAsset::CreateFromImage(pngPath.c_str(), tex);
    if (e != Error::ErrorCode::OK)
    {
        return e;
    }
    const GLuint glTex = CreateTexture(tex);
    textureBuffers.insert({name, glTex});
    return Error::ErrorCode::OK;
}

