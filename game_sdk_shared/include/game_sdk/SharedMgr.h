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

#include "libassets/util/SearchPathManager.h"

class SharedMgr
{
    public:
        static SharedMgr &Get();

        /**
         * Initialize the SDK shared manager
         */
        void InitSharedMgr();

        /**
         * Render the shared ImGui menu options
         * @param programName The program name, matching its entry on the Droc101 Development wiki
         */
        void SharedMenuUI(const std::string &programName);

        /**
         * Render shared ImGui windows
         */
        void RenderSharedUI();

        /**
         * Clean up the SDK shared manager
         */
        void DestroySharedMgr();

        void LoadActorDefinitions();

        void UpdateAssetPaths();

        GLTextureCache textureCache{};

        std::map<std::string, OptionDefinition> optionDefinitions{};

        std::map<std::string, ActorDefinition> actorDefinitions{};

        SearchPathManager spm{};

    private:
        bool metricsVisible = false;
        bool demoVisible = false;

        SharedMgr() = default;

        void LoadOptionDefinitions();
};
