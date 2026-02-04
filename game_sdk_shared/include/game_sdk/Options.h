//
// Created by droc101 on 6/29/25.
//

#pragma once

#include <cstdint>
#include <string>

class Options
{
    public:
        enum class Theme : uint8_t
        {
            SYSTEM,
            LIGHT,
            DARK
        };

        Options() = delete;

        /**
         * Load (or reload) options from disk
         */
        static void Load();

        /**
         * Load default options
         */
        static void LoadDefault();

        /**
         * Save options to disk
         */
        static void Save();

        /**
         * Do basic validation checks on the game path
         */
        static bool ValidateGamePath();

        /**
         * Get the absolute path to the assets folder
         */
        static std::string GetAssetsPath();

        static inline std::string gamePath;
        static inline bool overrideAssetsPath;
        static inline std::string assetsPath;

        static inline std::string defaultTexture;
        static inline std::string defaultMaterial;

        static inline Theme theme;
};
