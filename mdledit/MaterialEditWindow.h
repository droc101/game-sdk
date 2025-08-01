//
// Created by droc101 on 7/31/25.
//

#ifndef MATERIALEDITWINDOW_H
#define MATERIALEDITWINDOW_H



class MaterialEditWindow {
    public:
        MaterialEditWindow() = delete;

        static void Show();

        static void Hide();

        static void Render();

    private:
        static inline bool visible;
};



#endif //MATERIALEDITWINDOW_H
