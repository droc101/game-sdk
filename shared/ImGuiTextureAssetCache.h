//
// Created by droc101 on 7/18/25.
//

#pragma once

#include <imgui.h>
#include <libassets/util/Error.h>
#include <string>
#include <unordered_map>

class ImGuiTextureAssetCache
{
    public:
        ImGuiTextureAssetCache() = default;

        virtual ~ImGuiTextureAssetCache() = default;

        virtual Error::ErrorCode GetTextureID(const std::string & /*relPath*/, ImTextureID & /*outTexture*/) = 0;

        virtual Error::ErrorCode GetTextureSize(const std::string & /*relPath*/, ImVec2 & /*outSize*/) = 0;

    protected:
        std::unordered_map<std::string, ImTextureID> textureBuffers{};

        ImTextureID missingTexture{};
};
