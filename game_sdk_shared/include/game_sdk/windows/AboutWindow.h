//
// Created by droc101 on 6/29/25.
//

#pragma once

class AboutWindow
{
    public:
        static AboutWindow &Get();

        void Show();

        void Hide();

        void Render();

    private:
        AboutWindow() = default;

        bool visible = false;
};
