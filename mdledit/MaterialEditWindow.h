//
// Created by droc101 on 7/31/25.
//

#pragma once

class MaterialEditWindow
{
    public:
        MaterialEditWindow() = delete;

        static void Show();

        static void Hide();

        static void Render();

    private:
        static inline bool visible;
};
