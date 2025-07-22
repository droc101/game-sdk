//
// Created by droc101 on 7/18/25.
//

#include "SDLRendererImGuiTextureAssetCache.h"
#include <filesystem>
#include <ranges>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_surface.h>
#include "Options.h"
#include <libassets/asset/TextureAsset.h>

SDLRendererImGuiTextureAssetCache::SDLRendererImGuiTextureAssetCache(SDL_Renderer *renderer): ImGuiTextureAssetCache()
{
    this->renderer = renderer;
    SDL_Texture *sdlMissingTexture;
    if (!CreateSDLTexture(TextureAsset::CreateMissingTexture(), sdlMissingTexture)) throw std::runtime_error("Failed to create missing texture!");
    this->missingTexture = reinterpret_cast<ImTextureID>(sdlMissingTexture);
}

SDLRendererImGuiTextureAssetCache::~SDLRendererImGuiTextureAssetCache()
{
    for (const ImTextureID &val: textureBuffers | std::views::values)
    {
        SDL_Texture *t = reinterpret_cast<SDL_Texture *>(val);
        SDL_DestroyTexture(t);
    }
}

Error::ErrorCode SDLRendererImGuiTextureAssetCache::GetTextureID(const std::string &relPath, ImTextureID &outTexture)
{
    if (textureBuffers.contains(relPath))
    {
        outTexture = textureBuffers.at(relPath);
        return Error::ErrorCode::E_OK;
    }
    const std::string &texturePath = Options::gamePath + std::string("/assets/") + relPath;
    if (!std::filesystem::exists(texturePath))
    {
        outTexture = missingTexture;
        return Error::ErrorCode::E_OK;
    }

    TextureAsset asset;
    const Error::ErrorCode e = TextureAsset::CreateFromAsset(texturePath.c_str(), asset);
    if (e != Error::ErrorCode::E_OK) return e;
    SDL_Texture *tex;
    if (!CreateSDLTexture(asset, tex))
    {
        return Error::ErrorCode::E_UNKNOWN;
    }

    textureBuffers.insert({relPath, reinterpret_cast<ImTextureID>(tex)});
    outTexture = reinterpret_cast<ImTextureID>(tex);
    return Error::ErrorCode::E_OK;
}

Error::ErrorCode SDLRendererImGuiTextureAssetCache::GetTextureSize(const std::string &relPath, ImVec2 &outSize)
{
    ImTextureID tex;
    const Error::ErrorCode e = GetTextureID(relPath, tex);
    if (e != Error::ErrorCode::E_OK) return e;
    SDL_GetTextureSize(reinterpret_cast<SDL_Texture *>(tex), &outSize.x, &outSize.y);
    return Error::ErrorCode::E_OK;
}

bool SDLRendererImGuiTextureAssetCache::CreateSDLTexture(const TextureAsset& texture, SDL_Texture *&tex) const
{
    SDL_Surface *surface = SDL_CreateSurfaceFrom(static_cast<int>(texture.GetWidth()),
                                                 static_cast<int>(texture.GetHeight()),
                                                 SDL_PIXELFORMAT_RGBA8888,
                                                 const_cast<uint32_t *>(texture.GetPixels()),
                                                 static_cast<int>(texture.GetWidth() * sizeof(uint32_t)));
    if (surface == nullptr)
    {
        printf("SDL_CreateSurfaceFrom() failed: %s\n", SDL_GetError());
        return false;
    }
    tex = SDL_CreateTextureFromSurface(renderer, surface);
    if (tex == nullptr)
    {
        printf("SDL_CreateTextureFromSurface() failed: %s\n", SDL_GetError());
        return false;
    }
    SDL_SetTextureScaleMode(tex, SDL_SCALEMODE_NEAREST);
    SDL_DestroySurface(surface);
    return true;
}

