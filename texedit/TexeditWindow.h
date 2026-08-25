//
// Created by droc101 on 8/21/26.
//

#pragma once

#include <game_sdk/Window.h>
#include <GL/glew.h>
#include <imgui.h>
#include <libassets/asset/TextureAsset.h>
#include <SDL3/SDL_video.h>
#include <string>

class TexeditWindow final: public Window
{
    protected:
        bool Init() override;

        void Destroy() override;

        void Render() override;

        [[nodiscard]] const WindowProperties &GetProperties() const override;

    private:
        float zoom = 1.0f;
        ImVec2 pan = {0, 0};

        TextureAsset texture{};
        bool textureLoaded = false;
        GLuint glTexture = 0;

        bool showTransparencyCheckerboard = true;

        static constexpr const char *CHECKERBOARD_ICON_NAME = "editor/checkerboard";
        ImTextureID checkerboardTexture = 0;

        static constexpr float MIN_ZOOM = 0.1f;
        static constexpr float MAX_ZOOM = 10.0f;

        WindowProperties properties = {
            .title = "GAME SDK Texture Editor",
            .defaultSize = glm::ivec2(800, 600),
            .icon = "texedit",
            .defaultFlags = SDL_WINDOW_RESIZABLE,
            .defaultImguiWindow = true,
        };

        void DestroyExistingTexture();
        void LoadTexture();

        void OpenGtex(const std::string &path);
        void ImportImage(const std::string &path);
        void SaveGtex(const std::string &path);
        void Export(const std::string &path);

        void ClampZoom();
};
