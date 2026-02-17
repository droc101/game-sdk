//
// Created by droc101 on 2/17/26.
//

#ifndef GAME_SDK_MODELBROWSERWINDOW_H
#define GAME_SDK_MODELBROWSERWINDOW_H

#include <game_sdk/ModelViewer.h>
#include <string>
#include <vector>


class ModelBrowserWindow
{
    public:
        static ModelBrowserWindow &Get();

        void Show(std::string *model);
        void Hide();
        void Render();

        void InputModel(const char *label, std::string &model);

        void InputModel(const char *label, std::string *model);

    private:
        ModelBrowserWindow() = default;

        bool visible = false;
        std::string *str = nullptr;

        std::vector<std::string> models{};
        ModelViewer viewer{};
};


#endif //GAME_SDK_MODELBROWSERWINDOW_H
