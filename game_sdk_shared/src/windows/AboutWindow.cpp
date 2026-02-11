//
// Created by droc101 on 6/29/25.
//

#include <format>
#include <fstream>
#include <game_sdk/SDKWindow.h>
#include <game_sdk/SharedMgr.h>
#include <game_sdk/windows/AboutWindow.h>
#include <imgui.h>
#include <libassets/libassets.h>
#include <misc/cpp/imgui_stdlib.h>
#include <ranges>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

AboutWindow &AboutWindow::Get()
{
    static AboutWindow aboutWindowSingleton{};
    return aboutWindowSingleton;
}

void AboutWindow::Show()
{
    if (thirdPartyComponents.empty())
    {
        const std::vector<std::string> paths = SharedMgr::Get().ScanFolder("assets/licenses", "", true);
        for (const std::string &path: paths)
        {
            const std::ifstream file = std::ifstream("assets/licenses/" + path);
            std::stringstream buffer{};
            buffer << file.rdbuf();
            thirdPartyComponents[path] = buffer.str();
        }
        selectedComponent = std::views::keys(thirdPartyComponents).front();
    }
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
        ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_Appearing);
        ImGui::Begin("About the GAME SDK",
                     &visible,
                     ImGuiWindowFlags_NoCollapse |
                             ImGuiWindowFlags_NoSavedSettings |
                             ImGuiWindowFlags_NoDocking);
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
            visible = false;
        }

        ImGui::End();
    }
}
