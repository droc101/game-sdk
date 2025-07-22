//
// Created by droc101 on 7/22/25.
//

#ifndef OPENGLIMGUITEXTUREASSETCACHE_H
#define OPENGLIMGUITEXTUREASSETCACHE_H

#include "ImGuiTextureAssetCache.h"
#include <GL/glew.h>
#include <libassets/asset/TextureAsset.h>

class OpenGLImGuiTextureAssetCache final: public ImGuiTextureAssetCache
{
    public:
        OpenGLImGuiTextureAssetCache();

        ~OpenGLImGuiTextureAssetCache();

        [[nodiscard]] Error::ErrorCode GetTextureID(const std::string &relPath, ImTextureID &outTexture) override;

        [[nodiscard]] Error::ErrorCode GetTextureSize(const std::string &relPath, ImVec2 &outSize) override;

        [[nodiscard]] Error::ErrorCode GetTextureGLuint(const std::string &relPath, GLuint &outTexture);

    private:
        [[nodiscard]] static GLuint CreateTexture(const TextureAsset &textureAsset);
};


#endif //OPENGLIMGUITEXTUREASSETCACHE_H
