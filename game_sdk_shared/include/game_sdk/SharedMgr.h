//
// Created by droc101 on 7/1/25.
//

#pragma once

#include <game_sdk/gl/GLTextureCache.h>
#include <libassets/type/OptionDefinition.h>
#include <libassets/util/SearchPathManager.h>
#include <string>

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

        void UpdateAssetPaths();

        GLTextureCache textureCache{};

        SearchPathManager pathManager{};

    private:
        bool metricsVisible = false;
        bool demoVisible = false;

        SharedMgr() = default;
};
