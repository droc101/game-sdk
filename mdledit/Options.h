//
// Created by droc101 on 6/29/25.
//

#ifndef OPTIONS_H
#define OPTIONS_H
#include <array>


class Options {
    public:

        static void Load();
        static void Save();

        inline static std::array<char, 260> gamePath{};
};



#endif //OPTIONS_H
