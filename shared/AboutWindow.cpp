//
// Created by droc101 on 6/29/25.
//

#include "AboutWindow.h"
#include <format>
#include <imgui.h>
#include <libassets/libassets.h>

void AboutWindow::Show()
{
    visible = true;
}

void AboutWindow::Hide()
{
    visible = false;
}

void AboutWindow::Render()
{
    if (visible)
    {
        ImGui::SetNextWindowSize(ImVec2(300, -1));
        ImGui::Begin("About the GAME SDK",
                     &visible,
                     ImGuiWindowFlags_NoCollapse |
                             ImGuiWindowFlags_NoResize |
                             ImGuiWindowFlags_NoSavedSettings |
                             ImGuiWindowFlags_NoDocking);
        ImGui::TextUnformatted("Development & Authoring tools for");
        ImGui::SameLine();
        ImGui::TextLinkOpenURL("GAME", "https://github.com/droc101/c-game-engine");
        ImGui::TextUnformatted(std::format("Version {}", LIBASSETS_VERSION_STRING).c_str());

        ImGui::Dummy(ImVec2(0.0f, 16.0f));
        ImGui::TextUnformatted("Third-Party libraries:");
        ImGui::TextLinkOpenURL("SDL3", "https://www.libsdl.org");
        ImGui::TextLinkOpenURL("Dear ImGui", "https://github.com/ocornut/imgui");
        ImGui::TextLinkOpenURL("zlib", "https://zlib.net");
        ImGui::TextLinkOpenURL("JSON for Modern C++", "https://json.nlohmann.me");
        ImGui::TextLinkOpenURL("GLEW", "https://glew.sourceforge.net");
        ImGui::TextLinkOpenURL("assimp", "https://assimp.org/");
        ImGui::Dummy(ImVec2(0.0f, 16.0f));
        ImGui::Dummy(ImVec2(ImGui::GetContentRegionAvail().x - ImGui::GetStyle().WindowPadding.x - 60, 0));
        ImGui::SameLine();
        if (ImGui::Button("OK", ImVec2(60, 0)))
        {
            visible = false;
        }

        ImGui::End();
    }
}
