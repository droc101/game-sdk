//
// Created by droc101 on 7/22/25.
//

#include "OpenGLImGuiTextureAssetCache.h"
#include <filesystem>
#include "Options.h"

OpenGLImGuiTextureAssetCache::OpenGLImGuiTextureAssetCache()
{
    const GLuint glMissingTexture = CreateTexture(TextureAsset::CreateMissingTexture());
    this->missingTexture = static_cast<ImTextureID>(glMissingTexture);
}

OpenGLImGuiTextureAssetCache::~OpenGLImGuiTextureAssetCache()
{
    for (const std::pair<std::string, GLuint> tex: textureBuffers)
    {
        glDeleteTextures(1, &tex.second);
    }
    textureBuffers.clear();
}


GLuint OpenGLImGuiTextureAssetCache::CreateTexture(const TextureAsset &textureAsset)
{
    std::vector<uint32_t> pixels;
    textureAsset.GetPixelsRGBA(pixels);

    GLuint glTexture;
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
        return Error::ErrorCode::E_OK;
    }
    const std::string &texturePath = Options::gamePath + std::string("/assets/") + relPath;
    if (!std::filesystem::exists(texturePath))
    {
        outTexture = missingTexture;
        return Error::ErrorCode::E_OK;
    }
    TextureAsset asset;
    const Error::ErrorCode e = TextureAsset::CreateFromAsset(texturePath.c_str(), asset);
    if (e != Error::ErrorCode::E_OK) return e;
    const GLuint glTex = CreateTexture(asset);
    textureBuffers.insert({relPath, glTex});
    outTexture = glTex;
    return Error::ErrorCode::E_OK;
}

Error::ErrorCode OpenGLImGuiTextureAssetCache::GetTextureSize(const std::string &relPath, ImVec2 &outSize)
{
    ImTextureID t;
    const Error::ErrorCode e = GetTextureID(relPath, t);
    if (e != Error::ErrorCode::E_OK) return e;
    glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(t));
    int w;
    int h;
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &w);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &h);
    outSize = {static_cast<float>(w), static_cast<float>(h)};
    return Error::ErrorCode::E_OK;
}

Error::ErrorCode OpenGLImGuiTextureAssetCache::GetTextureGLuint(const std::string &relPath, GLuint &outTexture)
{
    ImTextureID t;
    const Error::ErrorCode e = GetTextureID(relPath, t);
    if (e != Error::ErrorCode::E_OK) return e;
    outTexture = static_cast<GLuint>(t);
    return Error::ErrorCode::E_OK;
}
