//
// Created by droc101 on 6/29/25.
//

#pragma once

#include <array>

class Options
{
    public:
        Options() = delete;

        static void Load();
        static void Save();

        static inline std::array<char, 260> gamePath{};
};
