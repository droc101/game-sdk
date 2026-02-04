//
// Created by droc101 on 7/1/25.
//

#pragma once

#include <game_sdk/gl/GLTextureCache.h>
#include <libassets/type/ActorDefinition.h>
#include <libassets/type/OptionDefinition.h>
#include <map>
#include <string>
#include <vector>

class SharedMgr
{
    public:
        SharedMgr() = delete;

        static void InitSharedMgr();

        static void SharedMenuUI(const std::string &programName);

        static void RenderSharedUI();

        static void DestroySharedMgr();

        static void ApplyTheme();

        static std::vector<std::string> ScanFolder(const std::string &directoryPath,
                                                   const std::string &extension,
                                                   bool isRoot);

        static void LoadOptionDefinitions();

        static void LoadActorDefinitions();

        static inline GLTextureCache textureCache{};

        static inline std::map<std::string, OptionDefinition> optionDefinitions{};

        static inline std::map<std::string, ActorDefinition> actorDefinitions{};

    private:
        static inline bool metricsVisible = false;
        static inline bool demoVisible = false;
};
