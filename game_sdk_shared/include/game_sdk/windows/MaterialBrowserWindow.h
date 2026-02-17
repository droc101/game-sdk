//
// Created by droc101 on 11/16/25.
//

#pragma once

#include <libassets/asset/LevelMaterialAsset.h>
#include <string>
#include <vector>

class MaterialBrowserWindow
{
    public:
        static MaterialBrowserWindow &Get();

        void Show(std::string *material);
        void Hide();
        void Render();

        void InputMaterial(const char *label, std::string &material);

        void InputMaterial(const char *label, std::string *material);

    private:
        MaterialBrowserWindow() = default;

        bool visible = false;
        std::string *str = nullptr;

        std::vector<std::string> materialPaths{};
        std::vector<LevelMaterialAsset> materials{};
};
