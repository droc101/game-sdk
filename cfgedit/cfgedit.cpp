//
// Created by droc101 on 10/4/25.
//

#include <format>
#include <game_sdk/DesktopInterface.h>
#include <game_sdk/DialogFilters.h>
#include <game_sdk/SDKWindow.h>
#include <game_sdk/SharedMgr.h>
#include <imgui.h>
#include <libassets/asset/GameConfigAsset.h>
#include <libassets/util/Error.h>
#include <misc/cpp/imgui_stdlib.h>
#include <string>

static GameConfigAsset config{};
static bool configLoaded = false;

static void openGame(const std::string &path)
{
    const Error::ErrorCode errorCode = GameConfigAsset::CreateFromAsset(path.c_str(), config);
    if (errorCode != Error::ErrorCode::OK)
    {
        SDKWindow::Get().ErrorMessage(std::format("Failed to open the game configuration!\n{}", errorCode));
        return;
    }
    configLoaded = true;
}

static void saveGame(const std::string &path)
{
    const Error::ErrorCode errorCode = config.SaveAsAsset(path.c_str());
    if (errorCode != Error::ErrorCode::OK)
    {
        SDKWindow::Get().ErrorMessage(std::format("Failed to save the game configuration!\n{}", errorCode));
    }
}

static void Render()
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
                SDKWindow::Get().PostQuit();
            }
            ImGui::EndMenu();
        }
        SharedMgr::Get().SharedMenuUI("cfgedit");
        ImGui::EndMainMenuBar();
    }

    if (openPressed)
    {
        SDKWindow::Get().OpenFileDialog(openGame, DialogFilters::gameFilters);
    } else if (savePressed)
    {
        SDKWindow::Get().SaveFileDialog(saveGame, DialogFilters::gameFilters);
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
    if (!SDKWindow::Get().Init("GAME SDK Game Config Editor", {400, 200}, 0))
    {
        return -1;
    }

    const std::string &openPath = DesktopInterface::Get().GetFileArgument(argc, argv, {".game"});
    if (!openPath.empty())
    {
        openGame(openPath);
    }

    SDKWindow::Get().MainLoop(Render);

    SDKWindow::Get().Destroy();

    return 0;
}
