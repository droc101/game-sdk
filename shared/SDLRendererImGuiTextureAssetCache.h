//
// Created by droc101 on 7/18/25.
//

#ifndef SDLRENDERERIMGUITEXTUREASSETCACHE_H
#define SDLRENDERERIMGUITEXTUREASSETCACHE_H
#include <SDL3/SDL_render.h>
#include "ImGuiTextureAssetCache.h"

class SDLRendererImGuiTextureAssetCache final: public ImGuiTextureAssetCache
{
    public:
        explicit SDLRendererImGuiTextureAssetCache(SDL_Renderer *renderer);

        ~SDLRendererImGuiTextureAssetCache() override;

        ImTextureID GetTextureID(const std::string &relPath) override;

        ImVec2 GetTextureSize(const std::string &relPath) override;

    private:
        SDL_Renderer *renderer = nullptr;

};


#endif //SDLRENDERERIMGUITEXTUREASSETCACHE_H
