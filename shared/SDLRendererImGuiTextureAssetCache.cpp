//
// Created by droc101 on 7/18/25.
//

#include "SDLRendererImGuiTextureAssetCache.h"
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <imgui.h>
#include <libassets/asset/TextureAsset.h>
#include <libassets/util/Error.h>
#include <ranges>
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_pixels.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_surface.h>
#include <stdexcept>
#include <string>
#include "ImGuiTextureAssetCache.h"
#include "Options.h"

SDLRendererImGuiTextureAssetCache::SDLRendererImGuiTextureAssetCache(SDL_Renderer *renderer): ImGuiTextureAssetCache()
{
    this->renderer = renderer;
    SDL_Texture *sdlMissingTexture = nullptr;
    TextureAsset missingTexture;
    TextureAsset::CreateMissingTexture(missingTexture);
    if (!CreateSDLTexture(missingTexture, sdlMissingTexture))
    {
        throw std::runtime_error("Failed to create missing texture!");
    }
    this->missingTexture = reinterpret_cast<ImTextureID>(sdlMissingTexture);
}

SDLRendererImGuiTextureAssetCache::~SDLRendererImGuiTextureAssetCache()
{
    for (const ImTextureID &val: textureBuffers | std::views::values)
    {
        SDL_Texture *texture = reinterpret_cast<SDL_Texture *>(val);
        SDL_DestroyTexture(texture);
    }
}

Error::ErrorCode SDLRendererImGuiTextureAssetCache::GetTextureID(const std::string &relPath, ImTextureID &outTexture)
{
    if (textureBuffers.contains(relPath))
    {
        outTexture = textureBuffers.at(relPath);
        return Error::ErrorCode::OK;
    }
    const std::string &texturePath = Options::GetAssetsPath() + "/" + relPath;
    if (!std::filesystem::exists(texturePath))
    {
        outTexture = missingTexture;
        return Error::ErrorCode::OK;
    }

    TextureAsset asset;
    const Error::ErrorCode e = TextureAsset::CreateFromAsset(texturePath.c_str(), asset);
    if (e != Error::ErrorCode::OK)
    {
        return e;
    }
    SDL_Texture *sdlTexture = nullptr;
    if (!CreateSDLTexture(asset, sdlTexture))
    {
        return Error::ErrorCode::UNKNOWN;
    }

    textureBuffers.insert({relPath, reinterpret_cast<ImTextureID>(sdlTexture)});
    outTexture = reinterpret_cast<ImTextureID>(sdlTexture);
    return Error::ErrorCode::OK;
}

Error::ErrorCode SDLRendererImGuiTextureAssetCache::GetTextureSize(const std::string &relPath, ImVec2 &outSize)
{
    ImTextureID tex = 0;
    const Error::ErrorCode e = GetTextureID(relPath, tex);
    if (e != Error::ErrorCode::OK)
    {
        return e;
    }
    if (!SDL_GetTextureSize(reinterpret_cast<SDL_Texture *>(tex), &outSize.x, &outSize.y))
    {
        printf("SDL_GetTextureSize() failed: %s\n", SDL_GetError());
        return Error::ErrorCode::UNKNOWN;
    }
    return Error::ErrorCode::OK;
}

bool SDLRendererImGuiTextureAssetCache::CreateSDLTexture(TextureAsset &texture, SDL_Texture *&sdlTexture) const
{
    SDL_Surface *surface = SDL_CreateSurfaceFrom(static_cast<int>(texture.GetWidth()),
                                                 static_cast<int>(texture.GetHeight()),
                                                 SDL_PIXELFORMAT_RGBA8888,
                                                 texture.GetPixels(),
                                                 static_cast<int>(texture.GetWidth() * sizeof(uint32_t)));
    if (surface == nullptr)
    {
        printf("SDL_CreateSurfaceFrom() failed: %s\n", SDL_GetError());
        return false;
    }
    sdlTexture = SDL_CreateTextureFromSurface(renderer, surface);
    if (sdlTexture == nullptr)
    {
        printf("SDL_CreateTextureFromSurface() failed: %s\n", SDL_GetError());
        return false;
    }
    if (!SDL_SetTextureScaleMode(sdlTexture, SDL_SCALEMODE_NEAREST))
    {
        printf("SDL_SetTextureScaleMode() failed: %s\n", SDL_GetError());
        SDL_DestroySurface(surface);
        return false;
    }
    SDL_DestroySurface(surface);
    return true;
}

Error::ErrorCode SDLRendererImGuiTextureAssetCache::RegisterPng(const std::string &pngPath, const std::string &name)
{
    TextureAsset tex;
    const Error::ErrorCode e = TextureAsset::CreateFromImage(pngPath.c_str(), tex);
    if (e != Error::ErrorCode::OK)
    {
        return e;
    }
    SDL_Texture *sdlTex = nullptr;
    if (!CreateSDLTexture(tex, sdlTex))
    {
        return Error::ErrorCode::UNKNOWN;
    }
    textureBuffers.insert({name, reinterpret_cast<ImTextureID>(sdlTex)});
    return Error::ErrorCode::OK;
}
