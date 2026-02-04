//
// Created by droc101 on 7/4/25.
//

#pragma once

#include <libassets/type/ModelLod.h>
#include <string>

class LodsTab
{
    public:
        LodsTab() = delete;

        static void Render();

    private:
        static inline ModelLod *lodToExport = nullptr;

        static void saveLodCallback(const std::string &path);
};
