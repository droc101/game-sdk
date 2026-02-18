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

        static Options &Get();

        /**
         * Load (or reload) options from disk
         */
        void Load();

        /**
         * Load default options
         */
        void LoadDefault();

        /**
         * Save options to disk
         */
        void Save();

        /**
         * Do basic validation checks on the game path
         */
        [[nodiscard]] bool ValidateGamePath() const;

        /**
         * Get the absolute path to the assets folder
         */
        [[nodiscard]] std::string GetAssetsPath() const;

        std::string gamePath{};
        bool overrideAssetsPath = false;
        std::string assetsPath{};

        std::string defaultTexture{};
        std::string defaultMaterial{};

        Theme theme = Theme::SYSTEM;
    private:
        Options() = default;
};
