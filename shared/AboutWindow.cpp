//
// Created by droc101 on 6/29/25.
//

#include "AboutWindow.h"
#include <imgui.h>
#include "libassets/libassets.h"

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
                     ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings);

        ImGui::Text("Development & Authoring tools for");
        ImGui::SameLine();
        ImGui::TextLinkOpenURL("GAME", "https://github.com/droc101/c-game-engine");
        ImGui::Text("Version %s", LIBASSETS_VERSION_STRING);

        ImGui::Dummy(ImVec2(0.0f, 16.0f));
        ImGui::Text("Third-Party libraries:");
        ImGui::TextLinkOpenURL("SDL3", "https://www.libsdl.org");
        if (ImGui::TextLink("Dear ImGui"))
        {
            imguiAboutVisible = true;
        }
        ImGui::TextLinkOpenURL("zlib", "https://zlib.net");
        ImGui::TextLinkOpenURL("JSON for Modern C++", "https://json.nlohmann.me");
        ImGui::TextLinkOpenURL("GLEW", "https://glew.sourceforge.net");
        ImGui::Dummy(ImVec2(0.0f, 16.0f));
        ImGui::Dummy(ImVec2(ImGui::GetContentRegionAvail().x - ImGui::GetStyle().WindowPadding.x - 60, 0));
        ImGui::SameLine();
        if (ImGui::Button("OK", ImVec2(60, 0)))
        {
            visible = false;
        }

        ImGui::End();
    }
    if (imguiAboutVisible)
    {
        ImGui::ShowAboutWindow(&imguiAboutVisible);
    }
}
