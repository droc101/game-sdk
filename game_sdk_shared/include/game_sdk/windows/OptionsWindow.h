//
// Created by droc101 on 6/29/25.
//

#pragma once

class OptionsWindow
{
    public:
        OptionsWindow() = delete;

        static void Show();

        static void Hide();

        static void Render();

    private:
        static inline bool visible = false;

        static void gamePathCallback(const std::string &path);

        static void assetsPathCallback(const std::string &path);
};
