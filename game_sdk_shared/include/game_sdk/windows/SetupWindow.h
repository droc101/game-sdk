//
// Created by droc101 on 11/12/25.
//

#pragma once

class SetupWindow
{
    public:
        SetupWindow() = delete;
        static void Show(bool required = true);
        static void Render();

    private:
        static inline bool visible = false;
        static inline bool required = true;

        static void gamePathCallback(const std::string &path);

        static void assetsPathCallback(const std::string &path);
};
