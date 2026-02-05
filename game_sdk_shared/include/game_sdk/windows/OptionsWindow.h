//
// Created by droc101 on 6/29/25.
//

#pragma once

class OptionsWindow
{
    public:
        static OptionsWindow &Get();

        void Show();

        void Hide();

        void Render();

    private:
        OptionsWindow() = default;

        bool visible = false;

        static void gamePathCallback(const std::string &path);

        static void assetsPathCallback(const std::string &path);
};
