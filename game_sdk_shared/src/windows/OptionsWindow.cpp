//
// Created by droc101 on 6/29/25.
//

#include <array>
#include <game_sdk/Options.h>
#include <game_sdk/SDKWindow.h>
#include <game_sdk/SharedMgr.h>
#include <game_sdk/windows/MaterialBrowserWindow.h>
#include <game_sdk/windows/OptionsWindow.h>
#include <game_sdk/windows/TextureBrowserWindow.h>
#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>

void OptionsWindow::Show()
{
    visible = true;
}

void OptionsWindow::Hide()
{
    visible = false;
}

void OptionsWindow::gamePathCallback(const std::string &path)
{
    Options::gamePath = path;
}

void OptionsWindow::assetsPathCallback(const std::string &path)
{
    Options::assetsPath = path;
}

void OptionsWindow::Render()
{
    if (visible)
    {
        ImGui::SetNextWindowSize(ImVec2(400, -1));
        ImGui::Begin("GAME SDK Options",
                     &visible,
                     ImGuiWindowFlags_NoCollapse |
                             ImGuiWindowFlags_NoResize |
                             ImGuiWindowFlags_NoSavedSettings |
                             ImGuiWindowFlags_NoDocking);
        ImGui::TextUnformatted("Folder with GAME executable");
        ImGui::SameLine();
        ImGui::TextDisabled("(no trailing slash)");
        ImGui::PushItemWidth(-ImGui::GetStyle().WindowPadding.x - 40);
        ImGui::InputText("##gamepathinput", &Options::gamePath);
        ImGui::SameLine();
        if (ImGui::Button("...", ImVec2(40, 0)))
        {
            SDKWindow::OpenFolderDialog(gamePathCallback);
        }

        ImGui::Checkbox("Override Assets Directory", &Options::overrideAssetsPath);
        ImGui::SameLine();
        ImGui::TextDisabled("(no trailing slash)");
        ImGui::BeginDisabled(!Options::overrideAssetsPath);
        ImGui::PushItemWidth(-ImGui::GetStyle().WindowPadding.x - 40);
        ImGui::InputText("##assetspathinput", &Options::assetsPath);
        ImGui::SameLine();
        if (ImGui::Button("...##assets", ImVec2(40, 0)))
        {
            SDKWindow::OpenFolderDialog(assetsPathCallback);
        }
        ImGui::EndDisabled();

        ImGui::TextUnformatted("Default Texture");
        TextureBrowserWindow::InputTexture("##defaulttexinput", Options::defaultTexture);
        ImGui::TextUnformatted("Default Material");
        MaterialBrowserWindow::InputMaterial("##defaultmatinput", Options::defaultMaterial);

        ImGui::TextUnformatted("Theme");
        int theme = static_cast<int>(Options::theme);
        const std::array<const char *, 3> options = {"System", "Light", "Dark"};
        if (ImGui::Combo("##theme", &theme, options.data(), 3))
        {
            Options::theme = static_cast<Options::Theme>(theme);
            SharedMgr::ApplyTheme();
        }

        ImGui::Dummy(ImVec2(0, 16));

        const float sizeX = ImGui::GetContentRegionAvail().x;

        ImGui::Dummy(ImVec2(sizeX - 120 - ImGui::GetStyle().WindowPadding.x - ImGui::GetStyle().WindowPadding.x, 0));
        ImGui::SameLine();
        if (ImGui::Button("OK", ImVec2(60, 0)))
        {
            Options::Save();
            visible = false;
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(60, 0)))
        {
            Options::Load();
            visible = false;
        }

        ImGui::End();
    }
}
