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

        /**
         * Create the fallback missing texture
         */
        void InitMissingTexture();

        /**
         * Get the ImGUI texture ID for a given path
         * @param relPath The texture path
         * @param outTexture Where to store the ID
         */
        [[nodiscard]] Error::ErrorCode GetTextureID(const std::string &relPath, ImTextureID &outTexture);

        /**
         * Get the size of a texture at a given path
         * @param relPath The texture path
         * @param outSize Where to store the size
         */
        [[nodiscard]] Error::ErrorCode GetTextureSize(const std::string &relPath, ImVec2 &outSize);

        /**
         * Get the OpenGL texture ID for a given path
         * @param relPath The texture path
         * @param outTexture Where to store the ID
         */
        [[nodiscard]] Error::ErrorCode GetTextureGLuint(const std::string &relPath, GLuint &outTexture);

        /**
         * Load a texture from a given path
         * @param relPath The path to load
         */
        [[nodiscard]] Error::ErrorCode LoadTexture(const std::string &relPath);

        /**
         * Register a PNG file as a texture with an arbitrary name
         * @param pngPath Path to the PNG file
         * @param name Name/path to register as
         */
        [[nodiscard]] Error::ErrorCode RegisterPng(const std::string &pngPath, const std::string &name);

    private:
        std::unordered_map<std::string, ImTextureID> textureBuffers{};

        ImTextureID missingTexture{};

        [[nodiscard]] static GLuint CreateTexture(const TextureAsset &textureAsset);
};
