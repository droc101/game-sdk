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

        static void GamePathCallback(const std::string &path);

        static void AssetsPathCallback(const std::string &path);
};
