//
// Created by droc101 on 10/4/25.
//

#include <cstdio>
#include <format>
#include <imgui.h>
#include <libassets/asset/GameConfigAsset.h>
#include <libassets/util/Error.h>
#include <misc/cpp/imgui_stdlib.h>
#include <SDL3/SDL_dialog.h>
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_messagebox.h>
#include <SDL3/SDL_video.h>
#include <string>
#include "DesktopInterface.h"
#include "DialogFilters.h"
#include "SDKWindow.h"
#include "SharedMgr.h"

static SDKWindow sdkWindow{};

static GameConfigAsset config{};
static bool configLoaded = false;

static void openGame(const std::string &path)
{
    const Error::ErrorCode errorCode = GameConfigAsset::CreateFromAsset(path.c_str(), config);
    if (errorCode != Error::ErrorCode::OK)
    {
        if (!SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
                                      "Error",
                                      std::format("Failed to open the gane configuration!\n{}", errorCode).c_str(),
                                      sdkWindow.GetWindow()))
        {
            printf("Failed to show SDL Error messagebox with error \"%s\"\n", SDL_GetError());
        }
        return;
    }
    configLoaded = true;
}

static void openGameCallback(void * /*userdata*/, const char *const *fileList, int /*filter*/)
{
    if (fileList == nullptr || fileList[0] == nullptr)
    {
        return;
    }
    openGame(fileList[0]);
}

static void saveGameCallback(void * /*userdata*/, const char *const *fileList, int /*filter*/)
{
    if (fileList == nullptr || fileList[0] == nullptr)
    {
        return;
    }
    const Error::ErrorCode errorCode = config.SaveAsAsset(fileList[0]);
    if (errorCode != Error::ErrorCode::OK)
    {
        if (!SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
                                      "Error",
                                      std::format("Failed to save the game configuration!\n{}", errorCode).c_str(),
                                      sdkWindow.GetWindow()))
        {
            printf("Failed to show SDL Error messagebox with error \"%s\"\n", SDL_GetError());
        }
    }
}

static void Render(SDL_Window *sdlWindow)
{
    const ImGuiViewport *viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    constexpr ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDecoration |
                                             ImGuiWindowFlags_NoMove |
                                             ImGuiWindowFlags_NoSavedSettings |
                                             ImGuiWindowFlags_NoBringToFrontOnFocus;
    ImGui::Begin("cfgedit", nullptr, windowFlags);
    bool newPressed = ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_N);
    bool openPressed = ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_O);
    bool savePressed = ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_S) && configLoaded;

    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            newPressed |= ImGui::MenuItem("New", "Ctrl+N");
            ImGui::Separator();
            openPressed |= ImGui::MenuItem("Open", "Ctrl+O");
            savePressed |= ImGui::MenuItem("Save", "Ctrl+S", false, configLoaded);
            ImGui::Separator();
            if (ImGui::MenuItem("Quit", "Alt+F4"))
            {
                sdkWindow.PostQuit();
            }
            ImGui::EndMenu();
        }
        SharedMgr::SharedMenuUI("cfgedit");
        ImGui::EndMainMenuBar();
    }

    if (openPressed)
    {
        SDL_ShowOpenFileDialog(openGameCallback,
                               nullptr,
                               sdlWindow,
                               DialogFilters::gameFilters.data(),
                               1,
                               nullptr,
                               false);
    } else if (savePressed)
    {
        SDL_ShowSaveFileDialog(saveGameCallback, nullptr, sdlWindow, DialogFilters::gameFilters.data(), 1, nullptr);
    } else if (newPressed)
    {
        config = GameConfigAsset();
        configLoaded = true;
    }

    if (configLoaded)
    {
        ImGui::PushItemWidth(-1);
        ImGui::Text("Game Title");
        ImGui::InputText("##gameTitle", &config.gameTitle);
        ImGui::Text("Game Copyright");
        ImGui::InputText("##gameCopyright", &config.gameCopyright);

        ImGui::Dummy({0, 12});
        ImGui::Separator();
        ImGui::Dummy({0, 12});

        ImGui::Text("Discord Rich Presence App ID");
        ImGui::SameLine();
        ImGui::TextDisabled("(set to 0 to disable)");
        ImGui::InputScalar("##appId", ImGuiDataType_U64, &config.discordAppId);
    } else
    {
        ImGui::TextDisabled("No game configuration is open.\nOpen or create one from the File menu.");
    }

    ImGui::End();
}

int main(int argc, char **argv)
{
    if (!sdkWindow.Init("cfgedit", {400, 200}, 0))
    {
        return -1;
    }

    const std::string &openPath = DesktopInterface::GetFileArgument(argc, argv, {".game"});
    if (!openPath.empty())
    {
        openGame(openPath);
    }

    sdkWindow.MainLoop(Render);

    return 0;
}
