//
// Created by droc101 on 7/22/25.
//

#pragma once

#include <GL/glew.h>
#include <libassets/asset/TextureAsset.h>
#include "ImGuiTextureAssetCache.h"

class OpenGLImGuiTextureAssetCache final: public ImGuiTextureAssetCache
{
    public:
        OpenGLImGuiTextureAssetCache();

        ~OpenGLImGuiTextureAssetCache() override;

        [[nodiscard]] Error::ErrorCode GetTextureID(const std::string &relPath, ImTextureID &outTexture) override;

        [[nodiscard]] Error::ErrorCode GetTextureSize(const std::string &relPath, ImVec2 &outSize) override;

        [[nodiscard]] Error::ErrorCode GetTextureGLuint(const std::string &relPath, GLuint &outTexture);

        [[nodiscard]] Error::ErrorCode LoadTexture(const std::string &relPath);

    private:
        [[nodiscard]] static GLuint CreateTexture(const TextureAsset &textureAsset);
};
