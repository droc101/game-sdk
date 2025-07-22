//
// Created by droc101 on 7/18/25.
//

#include "SDLRendererImGuiTextureAssetCache.h"
#include <ranges>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_surface.h>
#include "Options.h"
#include <libassets/asset/TextureAsset.h>

SDLRendererImGuiTextureAssetCache::SDLRendererImGuiTextureAssetCache(SDL_Renderer *renderer): ImGuiTextureAssetCache()
{
    this->renderer = renderer;
}

SDLRendererImGuiTextureAssetCache::~SDLRendererImGuiTextureAssetCache()
{
    for (const ImTextureID &val: textureBuffers | std::views::values)
    {
        SDL_Texture *t = reinterpret_cast<SDL_Texture *>(val);
        SDL_DestroyTexture(t);
    }
}

ImTextureID SDLRendererImGuiTextureAssetCache::GetTextureID(const std::string &relPath)
{
    if (textureBuffers.contains(relPath))
    {
        return textureBuffers.at(relPath);
    }
    // TODO: if the texture file does not exist return an existing missing texture instead of making a new one
    const std::string &texturePath = Options::gamePath + std::string("/assets/") + relPath;

    TextureAsset asset;
    [[maybe_unused]] const Error::ErrorCode e = TextureAsset::CreateFromAsset(texturePath.c_str(), asset);
    assert(e == Error::ErrorCode::E_OK);
    SDL_Surface *surface = SDL_CreateSurfaceFrom(static_cast<int>(asset.GetWidth()),
                                                 static_cast<int>(asset.GetHeight()),
                                                 SDL_PIXELFORMAT_RGBA8888,
                                                 const_cast<uint32_t *>(asset.GetPixels()),
                                                 static_cast<int>(asset.GetWidth() * sizeof(uint32_t)));
    if (surface == nullptr)
    {
        printf("SDL_CreateSurfaceFrom() failed: %s\n", SDL_GetError());
        return -1;
    }
    SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surface);
    if (tex == nullptr)
    {
        printf("SDL_CreateTextureFromSurface() failed: %s\n", SDL_GetError());
        return -1;
    }
    SDL_SetTextureScaleMode(tex, SDL_SCALEMODE_NEAREST);
    SDL_DestroySurface(surface);

    textureBuffers.insert({relPath, reinterpret_cast<ImTextureID>(tex)});
    return reinterpret_cast<ImTextureID>(tex);
}

ImVec2 SDLRendererImGuiTextureAssetCache::GetTextureSize(const std::string &relPath)
{
    SDL_Texture *tex = reinterpret_cast<SDL_Texture *>(GetTextureID(relPath));
    ImVec2 sz;
    SDL_GetTextureSize(tex, &sz.x, &sz.y);
    return sz;
}
