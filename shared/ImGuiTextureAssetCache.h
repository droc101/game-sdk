//
// Created by droc101 on 7/18/25.
//

#ifndef IMGUITEXTUREASSETCACHE_H
#define IMGUITEXTUREASSETCACHE_H
#include <stdexcept>
#include <string>
#include <unordered_map>
#include "imgui.h"

class ImGuiTextureAssetCache
{
    public:
        ImGuiTextureAssetCache() = default;

        virtual ~ImGuiTextureAssetCache() = default;

        virtual ImTextureID GetTextureID(const std::string & /*relPath*/)
        {
            throw std::runtime_error("Invalid use of ImGuiTextureAssetCache");
        };

        virtual ImVec2 GetTextureSize(const std::string &/*relPath*/)
        {
            throw std::runtime_error("Invalid use of ImGuiTextureAssetCache");
        };

    protected:
        std::unordered_map<std::string, ImTextureID> textureBuffers{};
        ImTextureID missingTexture{};
};


#endif //IMGUITEXTUREASSETCACHE_H
