//
// Created by droc101 on 6/13/26.
//

#include <cfloat>
#include <format>
#include <game_sdk/SDKWindow.h>
#include <game_sdk/SharedMgr.h>
#include <game_sdk/SoundSystem.h>
#include <game_sdk/windows/SoundBrowserWindow.h>
#include <imgui.h>
#include <libassets/asset/SoundAsset.h>
#include <libassets/util/Error.h>
#include <misc/cpp/imgui_stdlib.h>
#include <string>

SoundBrowserWindow &SoundBrowserWindow::Get()
{
    static SoundBrowserWindow soundBrowserWindowSingleton{};
    return soundBrowserWindowSingleton;
}

void SoundBrowserWindow::Hide()
{
    previewSound.Stop();
    SoundSystem::Get().UnloadSound(previewSound);
    previewSoundAsset = SoundAsset();
    visible = false;
}

void SoundBrowserWindow::Show(std::string *sound)
{
    str = sound;
    sounds.clear();
    sounds = SharedMgr::Get().pathManager.ScanAssetFolderR("/sound", ".gsnd");
    visible = true;
}

void SoundBrowserWindow::InputSound(const char *label, std::string &sound)
{
    InputSound(label, &sound);
}

void SoundBrowserWindow::InputSound(const char *label, std::string *sound)
{
    ImGui::PushItemWidth(-ImGui::GetStyle().WindowPadding.x - 40);
    ImGui::PushFont(SDKWindow::Get().GetMonospaceFont());
    ImGui::InputText(label, sound);
    ImGui::PopFont();
    ImGui::SameLine();
    if (ImGui::Button(("..." + std::string(label)).c_str(), ImVec2(40, 0)))
    {
        Show(sound);
    }
}

void SoundBrowserWindow::Render()
{
    if (visible)
    {
        ImGui::OpenPopup("Choose Sound");
        ImGui::SetNextWindowSize(ImVec2(600, 500), ImGuiCond_Appearing);
        ImGui::SetNextWindowSizeConstraints(ImVec2(192, 192), ImVec2(FLT_MAX, FLT_MAX));
        constexpr ImGuiWindowFlags WINDOW_FLAGS = ImGuiWindowFlags_NoCollapse |
                                                  ImGuiWindowFlags_NoSavedSettings |
                                                  ImGuiWindowFlags_NoDocking;
        if (ImGui::BeginPopupModal("Choose Sound", &visible, WINDOW_FLAGS))
        {
            ImGui::PushItemWidth(-1);
            ImGui::InputTextWithHint("##search", "Filter", &filter);
            ImGui::Dummy({0, 4});
            if (ImGui::BeginListBox("##picker", ImVec2(-1, -36)))
            {
                bool foundResults = false;

                for (const std::string &sound: sounds)
                {
                    if (sound.find(filter) == std::string::npos)
                    {
                        continue;
                    }

                    if (ImGui::Selectable(sound.c_str(), "sound/" + sound == *str))
                    {
                        *str = "sound/" + sound;
                    }

                    foundResults = true;
                }

                if (!foundResults)
                {
                    ImGui::Text("No results");
                }

                ImGui::EndListBox();
            }
            ImGui::Dummy({0, 4});
            if (ImGui::Button("Preview", ImVec2(60, 0)))
            {
                const Error::ErrorCode
                        loadError = SoundAsset::CreateFromAsset(SharedMgr::Get().pathManager.GetAssetPath(*str).c_str(),
                                                                previewSoundAsset);
                if (loadError != Error::ErrorCode::OK)
                {
                    SDKWindow::Get().ErrorMessage(std::format("Failed to open sound \"{}\": {}",
                                                              *str,
                                                              Error::ErrorString(loadError)));
                } else
                {
                    if (SoundSystem::Get().LoadSound(previewSoundAsset, previewSound))
                    {
                        previewSound.Play();
                    } else
                    {
                        SDKWindow::Get().ErrorMessage("Failed to create sound");
                    }
                }
            }
            ImGui::SameLine();
            ImGui::BeginDisabled(!previewSound.IsPlaying());
            if (ImGui::Button("Stop", ImVec2(60, 0)))
            {
                previewSound.Stop();
                SoundSystem::Get().UnloadSound(previewSound);
                previewSoundAsset = SoundAsset();
            }
            ImGui::EndDisabled();
            ImGui::SameLine();

            const float sizeX = ImGui::GetContentRegionAvail().x;

            ImGui::Dummy(ImVec2(sizeX - 60 - ImGui::GetStyle().WindowPadding.x, 0));
            ImGui::SameLine();
            if (ImGui::Button("OK", ImVec2(60, 0)))
            {
                Hide();
            }

            ImGui::EndPopup();
        }
    }
}
