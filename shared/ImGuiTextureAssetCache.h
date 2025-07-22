//
// Created by droc101 on 7/18/25.
//

#ifndef IMGUITEXTUREASSETCACHE_H
#define IMGUITEXTUREASSETCACHE_H
#include <stdexcept>
#include <string>
#include <unordered_map>
#include "imgui.h"
#include <libassets/util/Error.h>

class ImGuiTextureAssetCache
{
    public:
        ImGuiTextureAssetCache() = default;

        virtual ~ImGuiTextureAssetCache() = default;

        virtual Error::ErrorCode GetTextureID(const std::string & /*relPath*/, ImTextureID &/*outTexture*/)
        {
            throw std::runtime_error("Invalid use of ImGuiTextureAssetCache");
        };

        virtual Error::ErrorCode GetTextureSize(const std::string &/*relPath*/, ImVec2 &/*outSize*/)
        {
            throw std::runtime_error("Invalid use of ImGuiTextureAssetCache");
        };

    protected:
        std::unordered_map<std::string, ImTextureID> textureBuffers{};
        ImTextureID missingTexture{};


};


#endif //IMGUITEXTUREASSETCACHE_H
