//
// Created by droc101 on 9/29/26.
//

#ifndef GAME_SDK_COLORS_H
#define GAME_SDK_COLORS_H

#include <libassets/type/Color.h>

class Colors final
{
    public:
        Colors() = delete;

        static inline const Color WHITE = Color(1.0f, 1.0f, 1.0f, 1.0f);
        static inline const Color BLACK = Color(0.0f, 0.0f, 0.0f, 1.0f);
};

#endif //GAME_SDK_COLORS_H
