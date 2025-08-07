//
// Created by droc101 on 7/18/25.
//

#pragma once

#include <libassets/asset/TextureAsset.h>
#include <libassets/util/Error.h>
#include <SDL3/SDL_render.h>
#include "ImGuiTextureAssetCache.h"

class SDLRendererImGuiTextureAssetCache final: public ImGuiTextureAssetCache
{
    public:
        explicit SDLRendererImGuiTextureAssetCache(SDL_Renderer *renderer);

        ~SDLRendererImGuiTextureAssetCache() override;

        [[nodiscard]] Error::ErrorCode GetTextureID(const std::string &relPath, ImTextureID &outTexture) override;

        [[nodiscard]] Error::ErrorCode GetTextureSize(const std::string &relPath, ImVec2 &outSize) override;

    private:
        SDL_Renderer *renderer = nullptr;

        [[nodiscard]] bool CreateSDLTexture(TextureAsset &texture, SDL_Texture *&sdlTexture) const;
};
