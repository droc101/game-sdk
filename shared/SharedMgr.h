//
// Created by droc101 on 7/1/25.
//

#pragma once

#include <memory>
#include <SDL3/SDL_video.h>
#include <string>
#include <vector>
#include "ImGuiTextureAssetCache.h"
#include "Options.h"

class SharedMgr
{
    public:
        SharedMgr() = delete;

        template<typename T, typename... Args> static void InitSharedMgr(Args... args)
        {
            Options::Load();
            textureCache<T> = std::make_unique<T>(args...);
        }

        static void SharedMenuUI(const std::string &programName);

        static void RenderSharedUI(SDL_Window *window);

        static void DestroySharedMgr();

        static std::vector<std::string> ScanFolder(const std::string &directoryPath,
                                                   const std::string &extension,
                                                   bool isRoot);

        template<typename T> static inline std::unique_ptr<T> textureCache{};

    private:
        static inline bool metricsVisible = false;
        static inline bool demoVisible = false;
};
