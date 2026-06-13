#include <array>
#include <cmath>
#include <format>
#include <game_sdk/DesktopInterface.h>
#include <game_sdk/DialogFilters.h>
#include <game_sdk/SDKWindow.h>
#include <game_sdk/SharedMgr.h>
#include <game_sdk/SoundSystem.h>
#include <imgui.h>
#include <libassets/asset/SoundAsset.h>
#include <libassets/util/Error.h>
#include <string>

static SoundSystem::Sound sound{};
static SoundAsset soundAsset{};

static bool LoadSound()
{
    return SoundSystem::Get().LoadSound(soundAsset, sound);
}

static void OpenGsnd(const std::string &path)
{
    const Error::ErrorCode errorCode = SoundAsset::CreateFromAsset(path.c_str(), soundAsset);
    if (errorCode != Error::ErrorCode::OK)
    {
        SDKWindow::Get().ErrorMessage(std::format("Failed to open the sound!\n{}", errorCode));
        return;
    }
    if (!LoadSound())
    {
        SDKWindow::Get().ErrorMessage(std::format("Failed to load the sound!\n{}", Error::ErrorCode::UNKNOWN));
    }
}

static void ImportWav(const std::string &path)
{
    const Error::ErrorCode errorCode = SoundAsset::CreateFromWAV(path.c_str(), soundAsset);
    if (errorCode != Error::ErrorCode::OK)
    {
        SDKWindow::Get().ErrorMessage(std::format("Failed to import the sound!\n{}", errorCode));
        return;
    }
    if (!LoadSound())
    {
        SDKWindow::Get().ErrorMessage(std::format("Failed to load the sound!\n{}", Error::ErrorCode::UNKNOWN));
    }
}

static void SaveGsnd(const std::string &path)
{
    const Error::ErrorCode errorCode = soundAsset.SaveAsAsset(path.c_str());
    if (errorCode != Error::ErrorCode::OK)
    {
        SDKWindow::Get().ErrorMessage(std::format("Failed to save the sound!\n{}", errorCode));
    }
}

static void ExportWav(const std::string &path)
{
    const Error::ErrorCode errorCode = soundAsset.SaveAsWAV(path.c_str());
    if (errorCode != Error::ErrorCode::OK)
    {
        SDKWindow::Get().ErrorMessage(std::format("Failed to export the sound!\n{}", errorCode));
    }
}

static void Render()
{
    const ImGuiViewport *viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    constexpr ImGuiWindowFlags WINDOW_FLAGS = ImGuiWindowFlags_NoDecoration |
                                              ImGuiWindowFlags_NoMove |
                                              ImGuiWindowFlags_NoSavedSettings |
                                              ImGuiWindowFlags_NoBringToFrontOnFocus;
    ImGui::Begin("sndedit", nullptr, WINDOW_FLAGS);
    bool openPressed = ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_O);
    bool importPressed = ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_O);
    bool savePressed = ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_S) && sound.IsLoaded();
    bool exportPressed = ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_S) && sound.IsLoaded();

    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            openPressed |= ImGui::MenuItem("Open", "Ctrl+O");
            importPressed |= ImGui::MenuItem("Import", "Ctrl+Shift+O");
            savePressed |= ImGui::MenuItem("Save", "Ctrl+S", false, sound.IsLoaded());
            exportPressed |= ImGui::MenuItem("Export", "Ctrl+Shift+S", false, sound.IsLoaded());
            ImGui::Separator();
            if (ImGui::MenuItem("Quit", "Alt+F4"))
            {
                SDKWindow::Get().PostQuit();
            }
            ImGui::EndMenu();
        }
        SharedMgr::Get().SharedMenuUI("sndedit");
        ImGui::EndMainMenuBar();
    }

    if (openPressed)
    {
        SDKWindow::Get().OpenFileDialog(OpenGsnd, DialogFilters::GSND_FILTERS);
    } else if (importPressed)
    {
        SDKWindow::Get().OpenFileDialog(ImportWav, DialogFilters::WAV_FILTERS);
    } else if (savePressed)
    {
        SDKWindow::Get().SaveFileDialog(SaveGsnd, DialogFilters::GSND_FILTERS);
    } else if (exportPressed)
    {
        SDKWindow::Get().SaveFileDialog(ExportWav, DialogFilters::WAV_FILTERS);
    }

    if (sound.IsLoaded())
    {
        const float length = sound.GetLength();
        const float cursor = sound.GetCursor();
        const bool isPlaying = sound.IsPlaying();
        bool isLooping = sound.IsLooping();

        ImGui::SeparatorText("Sound Player");

        ImGui::PushItemWidth(-1);
        float nCursor = cursor;
        if (ImGui::SliderFloat("##Seek", &nCursor, 0.0f, length, ""))
        {
            if (nCursor != cursor)
            {
                sound.Seek(nCursor);
                if (!isPlaying)
                {
                    sound.Play();
                }
            }
        }

        if (!isPlaying)
        {
            if (ImGui::Button("Play", ImVec2(50, 0)))
            {
                sound.Play();
            }
        } else
        {
            if (ImGui::Button("Pause", ImVec2(50, 0)))
            {
                sound.Stop();
            }
        }

        ImGui::SameLine();
        ImGui::PushItemWidth(100);
        float volume = SoundSystem::Get().GetVolume() * 100.0f;
        if (ImGui::SliderFloat("##Volume", &volume, 0.0f, 100.0f, "%.0f%%"))
        {
            SoundSystem::Get().SetVolume(volume / 100.0f);
        }

        const float cursorSeconds = std::fmod(cursor, 60.0f);
        const float cursorMinutes = std::floor(cursor / 60.0f);
        const float lengthSeconds = std::fmod(length, 60.0f);
        const float lengthMinutes = std::floor(length / 60.0f);
        ImGui::SameLine();
        ImGui::Text("%g:%05.2f / %g:%05.2f", cursorMinutes, cursorSeconds, lengthMinutes, lengthSeconds);


        const float availWidth = ImGui::GetContentRegionAvail().x;
        const float checkboxWidth = ImGui::CalcTextSize("Loop").x + ImGui::GetFrameHeight();
        ImGui::SameLine();
        ImGui::SetCursorPosX(availWidth - checkboxWidth);
        if (ImGui::Checkbox("Loop", &isLooping))
        {
            sound.SetLooping(isLooping);
        }

        ImGui::SeparatorText("Sound Information");
        ImGui::TextUnformatted(std::format("Size: {} bytes", soundAsset.GetDataSize()).c_str());
        SoundSystem::Sound::Format format = sound.GetFormat();
        ma_uint64 pcmLen = sound.GetLengthInFrames();
        constexpr std::array<const char *, 6> FORMAT_NAMES = {"Unknown", "U8", "S16", "S24", "S32", "F32"};
        ImGui::TextUnformatted(std::format("Format: {}", FORMAT_NAMES.at(format.format)).c_str());
        ImGui::TextUnformatted(std::format("Channels: {}", format.channels).c_str());
        ImGui::TextUnformatted(std::format("Sample Rate: {} Hz", format.sampleRate).c_str());
        ImGui::Text("Length: %g:%05.2f", lengthMinutes, lengthSeconds);
        ImGui::TextUnformatted(std::format("Length (PCM frames): {}", pcmLen).c_str());

    } else
    {
        ImGui::TextDisabled("No sound is open. Open or import one from the File menu.");
    }

    ImGui::End();
}

int main(const int argc, char **argv)
{
    if (!SDKWindow::Get().Init("GAME SDK Sound Editor"))
    {
        return -1;
    }

    SDKWindow::Get().SetWindowIcon("sndedit");

    if (!SoundSystem::Get().Init())
    {
        return -1;
    }

    const std::string &openPath = DesktopInterface::Get().GetFileArgument(argc, argv, {".gsnd"});
    if (!openPath.empty())
    {
        OpenGsnd(openPath);
    } else
    {
        const std::string &importPath = DesktopInterface::Get().GetFileArgument(argc, argv, {".wav"});
        if (!importPath.empty())
        {
            ImportWav(importPath);
        }
    }

    SDKWindow::Get().MainLoop(Render);

    SoundSystem::Get().Destroy();

    SDKWindow::Get().Destroy();

    return 0;
}
