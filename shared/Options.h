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

        static void Load();

        static void LoadDefault();

        static void Save();

        static bool ValidateGamePath();

        static inline std::string gamePath;

        static inline std::string defaultTexture;
        static inline std::string defaultMaterial;

        static inline Theme theme;
};
