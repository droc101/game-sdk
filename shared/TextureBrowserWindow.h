//
// Created by droc101 on 7/6/25.
//

#pragma once

#include <string>
#include <vector>

class TextureBrowserWindow
{
    public:
        TextureBrowserWindow() = delete;

        static void Show(std::string &texture);
        static void Hide();
        static void Render();

        static void InputTexture(const char *label, std::string &texture);

    private:
        static inline bool visible = false;
        static inline std::string *str = nullptr;

        static inline std::vector<std::string> textures;
};
