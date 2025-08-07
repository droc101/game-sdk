//
// Created by droc101 on 6/29/25.
//

#pragma once

#include <string>

class Options
{
    public:
        Options() = delete;

        static void Load();

        static void LoadDefault();

        static void Save();

        static inline std::string gamePath;

        static inline std::string defaultTexture;
};
