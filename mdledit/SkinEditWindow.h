//
// Created by droc101 on 7/1/25.
//

#ifndef SKINEDITWINDOW_H
#define SKINEDITWINDOW_H

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


#endif //SKINEDITWINDOW_H
