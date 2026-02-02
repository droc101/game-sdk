//
// Created by droc101 on 7/31/25.
//

#pragma once

#include <cstddef>

class MaterialsTab
{
    public:
        MaterialsTab() = delete;

        static void Render();

    private:
        static inline size_t selectedIndex = 0;
};
