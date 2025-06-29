//
// Created by droc101 on 6/29/25.
//

#include "AboutWindow.h"
#include "imgui.h"
#include "libassets/libassets.h"

void AboutWindow::Show()
{
    visible = true;
}

void AboutWindow::Hide()
{
    visible = false;
}

void AboutWindow::Render(SDL_Window *window)
{
    if (visible)
    {
        ImGui::SetNextWindowSize(ImVec2(300, -1));
        ImGui::Begin("About the GAME SDK", &visible, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings);

        ImGui::Text("Development & Authoring tools for");
        ImGui::SameLine();
        ImGui::TextLinkOpenURL("GAME", "https://github.com/droc101/c-game-engine");
        ImGui::Text("Version %s", LIBASSETS_VERSION_STRING);

        ImGui::Dummy(ImVec2(0.0f, 16.0f));
        ImGui::Text("Third-Party libraries:");
        ImGui::TextLinkOpenURL("SDL3", "https://www.libsdl.org");
        ImGui::TextLinkOpenURL("Dear ImGui", "https://www.dearimgui.com");
        ImGui::TextLinkOpenURL("zlib", "https://zlib.net");
        ImGui::TextLinkOpenURL("JSON for Modern C++", "https://json.nlohmann.me");
        ImGui::Dummy(ImVec2(0.0f, 16.0f));
        if (ImGui::Button("OK", ImVec2(60, 0))) visible = false;

        ImGui::End();
    }
}
