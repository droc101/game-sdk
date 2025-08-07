//
// Created by droc101 on 7/1/25.
//

#pragma once

class SkinEditWindow
{
    public:
        SkinEditWindow() = delete;

        static void Show();

        static void Hide();

        static void Render();

    private:
        static inline bool visible;
};
