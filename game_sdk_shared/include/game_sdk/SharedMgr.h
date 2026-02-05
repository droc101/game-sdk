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

        /**
         * Apply the ImGui theme from options
         */
        void ApplyTheme();

        /**
         * Recursively scan a directory for files of a certain type
         * @param directoryPath The path to the directory
         * @param extension The extension to scan for
         * @param isRoot Set this to true
         * @return A vector of file paths
         */
        std::vector<std::string> ScanFolder(const std::string &directoryPath,
                                                   const std::string &extension,
                                                   bool isRoot);

        GLTextureCache textureCache{};

        std::map<std::string, OptionDefinition> optionDefinitions{};

        std::map<std::string, ActorDefinition> actorDefinitions{};

    private:
        bool metricsVisible = false;
        bool demoVisible = false;

        SharedMgr() = default;

        void LoadOptionDefinitions();

        void LoadActorDefinitions();
};
