//
// Created by droc101 on 7/18/25.
//

#pragma once

#include <GL/glew.h>
#include <imgui.h>
#include <libassets/asset/TextureAsset.h>
#include <libassets/util/Error.h>
#include <string>
#include <unordered_map>

class GLTextureCache
{
    public:
        GLTextureCache() = default;

        ~GLTextureCache();

        void InitMissingTexture();

        [[nodiscard]] Error::ErrorCode GetTextureID(const std::string &relPath, ImTextureID &outTexture);

        [[nodiscard]] Error::ErrorCode GetTextureSize(const std::string &relPath, ImVec2 &outSize);

        [[nodiscard]] Error::ErrorCode GetTextureGLuint(const std::string &relPath, GLuint &outTexture);

        [[nodiscard]] Error::ErrorCode LoadTexture(const std::string &relPath);

        [[nodiscard]] Error::ErrorCode RegisterPng(const std::string &pngPath, const std::string &name);

    private:
        std::unordered_map<std::string, ImTextureID> textureBuffers{};

        ImTextureID missingTexture{};

        [[nodiscard]] static GLuint CreateTexture(const TextureAsset &textureAsset);
};
