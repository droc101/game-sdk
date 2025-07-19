//
// Created by droc101 on 7/1/25.
//

#ifndef SHAREDUIMGR_H
#define SHAREDUIMGR_H
#include <string>
#include <vector>
#include <SDL3/SDL_video.h>
#include "ImGuiTextureAssetCache.h"

class SharedMgr
{
    public:
        SharedMgr() = delete;

        static void InitSharedMgr(ImGuiTextureAssetCache *cache);

        static void SharedMenuUI();

        static void RenderSharedUI(SDL_Window *window);

        static void DestroySharedMgr();

        static std::vector<std::string> ScanFolder(const std::string &directory_path, const std::string &extension);

        inline static ImGuiTextureAssetCache *textureCache = nullptr;

    private:
        inline static bool metricsVisible = false;
        inline static bool demoVisible = false;
};


#endif //SHAREDUIMGR_H
