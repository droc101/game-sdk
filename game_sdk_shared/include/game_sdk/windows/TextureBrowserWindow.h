//
// Created by droc101 on 7/6/25.
//

#pragma once

#include <string>
#include <vector>

class TextureBrowserWindow
{
    public:
        static TextureBrowserWindow &Get();

        void Show(std::string &texture);
        void Hide();
        void Render();

        void InputTexture(const char *label, std::string &texture);

    private:
        TextureBrowserWindow() = default;

        bool visible = false;
        std::string *str = nullptr;

        std::vector<std::string> textures;
};
