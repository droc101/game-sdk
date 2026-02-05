//
// Created by droc101 on 11/12/25.
//

#include <cstdlib>
#include <game_sdk/Options.h>
#include <game_sdk/SDKWindow.h>
#include <game_sdk/windows/SetupWindow.h>
#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>


SetupWindow &SetupWindow::Get()
{
    static SetupWindow setupWindowSingleton{};
    return setupWindowSingleton;
}

void SetupWindow::Show(bool required)
{
    visible = true;
    SetupWindow::required = required;
}

void SetupWindow::gamePathCallback(const std::string &path)
{
    Options::Get().gamePath = path;
}

void SetupWindow::assetsPathCallback(const std::string &path)
{
    Options::Get().assetsPath = path;
}

void SetupWindow::Render()
{
    if (!visible)
    {
        return;
    }
    ImGui::OpenPopup("Setup");
    const ImGuiViewport *viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    constexpr ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoMove |
                                             ImGuiWindowFlags_NoSavedSettings |
                                             ImGuiWindowFlags_NoCollapse |
                                             ImGuiWindowFlags_NoResize |
                                             ImGuiWindowFlags_NoDocking;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    if (!ImGui::BeginPopupModal("Setup", nullptr, windowFlags))
    {
        return;
    }
    ImGui::PopStyleVar();

    ImGui::TextUnformatted("Folder with GAME executable");
    ImGui::SameLine();
    ImGui::TextDisabled("(no trailing slash)");
    ImGui::PushItemWidth(-ImGui::GetStyle().WindowPadding.x - 40);
    ImGui::InputText("##gamepathinput", &Options::Get().gamePath);
    ImGui::SameLine();
    if (ImGui::Button("...", ImVec2(40, 0)))
    {
        SDKWindow::Get().OpenFolderDialog(gamePathCallback);
    }
    bool valid = true;
    if (!Options::Get().ValidateGamePath())
    {
        valid = false;
        ImGui::TextColored(ImVec4(1, 0, 0, 1), "Invalid game path");
    } else
    {
        ImGui::Text(" ");
    }

    ImGui::Checkbox("Override Assets Directory", &Options::Get().overrideAssetsPath);
    ImGui::SameLine();
    ImGui::TextDisabled("(no trailing slash)");
    ImGui::BeginDisabled(!Options::Get().overrideAssetsPath);
    ImGui::PushItemWidth(-ImGui::GetStyle().WindowPadding.x - 40);
    ImGui::InputText("##assetspathinput", &Options::Get().assetsPath);
    ImGui::SameLine();
    if (ImGui::Button("...##assets", ImVec2(40, 0)))
    {
        SDKWindow::Get().OpenFolderDialog(assetsPathCallback);
    }
    ImGui::EndDisabled();


    ImGui::Dummy(ImVec2(0, 16));
    const float sizeX = ImGui::GetContentRegionAvail().x;

    ImGui::Dummy(ImVec2(sizeX - 120 - ImGui::GetStyle().WindowPadding.x - ImGui::GetStyle().WindowPadding.x, 0));
    ImGui::SameLine();
    ImGui::BeginDisabled(!valid);
    if (ImGui::Button("OK", ImVec2(60, 0)))
    {
        Options::Get().Save();
        visible = false;
    }
    ImGui::EndDisabled();
    ImGui::SameLine();
    if (required)
    {
        if (ImGui::Button("Quit", ImVec2(60, 0)))
        {
            Options::Get().Load();
            exit(1);
        }
    } else
    {
        if (ImGui::Button("Cancel", ImVec2(60, 0)))
        {
            Options::Get().Load();
            visible = false;
        }
    }

    ImGui::EndPopup();
}
