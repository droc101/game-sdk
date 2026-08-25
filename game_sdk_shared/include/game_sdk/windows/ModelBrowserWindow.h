//
// Created by droc101 on 2/17/26.
//

#ifndef GAME_SDK_MODELBROWSERWINDOW_H
#define GAME_SDK_MODELBROWSERWINDOW_H

#include <game_sdk/ModelViewer.h>
#include <game_sdk/Window.h>
#include <string>
#include <vector>

class ModelBrowserWindow final: public Window
{
    public:
        ModelBrowserWindow(std::string *model);

        static void InputModel(const char *label, std::string &model);

        static void InputModel(const char *label, std::string *model);

        static void Show(std::string *texture);

    protected:
        bool Init() override;

        void Destroy() override;

        void Render() override;

        const WindowProperties &GetProperties() const override;

    private:
        WindowProperties properties = {
            .title = "Choose Model",
            .defaultSize = glm::ivec2(1366, 768),
            .icon = "",
            .defaultFlags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_UTILITY,
            .defaultImguiWindow = true,
        };

        std::string *str = nullptr;

        std::vector<std::string> models{};
        std::vector<std::string> modelAbsPaths{};
        ModelViewer viewer{};

        std::string filter;
};


#endif //GAME_SDK_MODELBROWSERWINDOW_H
