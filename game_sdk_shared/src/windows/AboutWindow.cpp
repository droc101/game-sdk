//
// Created by droc101 on 6/29/25.
//

#include <format>
#include <game_sdk/SDKWindow.h>
#include <game_sdk/windows/AboutWindow.h>
#include <imgui.h>
#include <libassets/libassets.h>
#include <libassets/util/FileIo.h>
#include <libassets/util/SearchPathManager.h>
#include <misc/cpp/imgui_stdlib.h>
#include <ranges>
#include <string>
#include <vector>

const Window::WindowProperties &AboutWindow::GetProperties() const
{
    return properties;
}

bool AboutWindow::Init()
{
    if (thirdPartyComponents.empty())
    {
        const std::vector<std::string> paths = SearchPathManager::ScanFolder("assets/licenses", "", true);
        for (const std::string &path: paths)
        {
            std::string text;
            const Error::ErrorCode readError = FileIo::ReadFileToString("assets/licenses/" + path, text);
            if (readError != Error::ErrorCode::OK)
            {
                continue;
            }
            thirdPartyComponents[path] = text;
        }
        selectedComponent = std::views::keys(thirdPartyComponents).front();
    }
    return true;
}

void AboutWindow::Render()
{
    ImGui::TextUnformatted("Development & Authoring tools for");
    ImGui::SameLine();
    ImGui::TextLinkOpenURL("GAME", "https://github.com/droc101/c-game-engine");
    ImGui::TextUnformatted(std::format("Version {}", LIBASSETS_VERSION_STRING).c_str());

    const float s = ImGui::GetContentRegionAvail().y - 64;

    ImGui::SeparatorText("Third-Party Components");
    if (ImGui::BeginListBox("##thirdParty", ImVec2(150, s)))
    {
        for (const std::string &component: std::views::keys(thirdPartyComponents))
        {
            if (ImGui::Selectable(component.c_str(), selectedComponent == component))
            {
                selectedComponent = component;
            }
        }
        ImGui::EndListBox();
    }
    ImGui::SameLine();
    ImGui::PushFont(SDKWindow::Get().GetMonospaceFont(), 18);
    ImGui::InputTextMultiline("##glsl",
                              &thirdPartyComponents[selectedComponent],
                              ImVec2(-1, s),
                              ImGuiInputTextFlags_ReadOnly |
                                      ImGuiInputTextFlags_WordWrap |
                                      ImGuiInputTextFlags_NoHorizontalScroll);
    ImGui::PopFont();

    ImGui::Separator();
    ImGui::Dummy(ImVec2(ImGui::GetContentRegionAvail().x - ImGui::GetStyle().WindowPadding.x - 60, 0));
    ImGui::SameLine();
    if (ImGui::Button("OK", ImVec2(60, 0)))
    {
        closeRequest = true;
    }
}
