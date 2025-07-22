//
// Created by droc101 on 7/18/25.
//

#ifndef SDLRENDERERIMGUITEXTUREASSETCACHE_H
#define SDLRENDERERIMGUITEXTUREASSETCACHE_H
#include <SDL3/SDL_render.h>
#include "ImGuiTextureAssetCache.h"
#include "libassets/asset/TextureAsset.h"
#include "libassets/util/Error.h"

class SDLRendererImGuiTextureAssetCache final: public ImGuiTextureAssetCache
{
    public:
        explicit SDLRendererImGuiTextureAssetCache(SDL_Renderer *renderer);

        ~SDLRendererImGuiTextureAssetCache() override;

        [[nodiscard]] Error::ErrorCode GetTextureID(const std::string &relPath, ImTextureID &outTexture) override;

        [[nodiscard]] Error::ErrorCode GetTextureSize(const std::string &relPath, ImVec2 &outSize) override;

    private:
        SDL_Renderer *renderer = nullptr;

        [[nodiscard]] bool CreateSDLTexture(const TextureAsset& texture, SDL_Texture *&tex) const;

};


#endif //SDLRENDERERIMGUITEXTUREASSETCACHE_H
