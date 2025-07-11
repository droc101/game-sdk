//
// Created by droc101 on 6/29/25.
//

#pragma once

class AboutWindow
{
    public:
        AboutWindow() = delete;

        static void Show();
        static void Hide();
        static void Render();

    private:
        static inline bool visible = false;
};
