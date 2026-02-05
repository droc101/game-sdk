//
// Created by droc101 on 11/12/25.
//

#pragma once

class SetupWindow
{
    public:
        static SetupWindow &Get();

        void Show(bool required = true);
        void Render();

    private:
        SetupWindow() = default;

        bool visible = false;
        bool required = true;

        static void gamePathCallback(const std::string &path);

        static void assetsPathCallback(const std::string &path);
};
