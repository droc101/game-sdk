//
// Created by droc101 on 11/16/25.
//

#pragma once

#include <string>
#include <vector>

#include "libassets/asset/LevelMaterialAsset.h"

class MaterialBrowserWindow
{
    public:
        MaterialBrowserWindow() = delete;

        static void Show(std::string &material);
        static void Hide();
        static void Render();

        static void InputMaterial(const char *label, std::string &material);

    private:
        static inline bool visible = false;
        static inline std::string *str = nullptr;

        static inline std::vector<std::string> materialPaths;
        static inline std::vector<LevelMaterialAsset> materials;
};
