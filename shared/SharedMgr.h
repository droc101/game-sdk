//
// Created by droc101 on 7/1/25.
//

#pragma once

#include <libassets/type/ActorDefinition.h>
#include <libassets/type/OptionDefinition.h>
#include <memory>
#include <SDL3/SDL_video.h>
#include <string>
#include <unordered_map>
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
            textureCache = std::make_unique<T>(args...);
            LoadOptionDefinitions();
            LoadActorDefinitions();
        }

        static void SharedMenuUI(const std::string &programName);

        static void RenderSharedUI(SDL_Window *window);

        static void DestroySharedMgr();

        static void ApplyTheme();

        static std::vector<std::string> ScanFolder(const std::string &directoryPath,
                                                   const std::string &extension,
                                                   bool isRoot);

        static void LoadOptionDefinitions();

        static void LoadActorDefinitions();

        static inline std::unique_ptr<ImGuiTextureAssetCache> textureCache{};

        static inline std::unordered_map<std::string, OptionDefinition> optionDefinitions{};

        static inline std::unordered_map<std::string, ActorDefinition> actorDefinitions{};

    private:
        static inline bool metricsVisible = false;
        static inline bool demoVisible = false;
};
