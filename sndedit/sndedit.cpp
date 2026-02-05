#include <array>
#include <cmath>
#include <cstdio>
#include <format>
#include <game_sdk/DesktopInterface.h>
#include <game_sdk/DialogFilters.h>
#include <game_sdk/SDKWindow.h>
#include <game_sdk/SharedMgr.h>
#include <imgui.h>
#include <libassets/asset/SoundAsset.h>
#include <libassets/util/Error.h>
#include <miniaudio.h>
#include <string>

static ma_engine engine{};
static ma_decoder decoder{};

static SoundAsset soundAsset{};
static ma_sound sound{};
static bool soundLoaded = false;

static void destroyExistingSound()
{
    if (!soundLoaded)
    {
        return;
    }
    ma_sound_stop(&sound);
    ma_sound_uninit(&sound);
    ma_decoder_uninit(&decoder);
    soundLoaded = false;
}

static bool loadSound()
{
    destroyExistingSound();
    ma_result result = ma_decoder_init_memory(soundAsset.GetData().data(), soundAsset.GetDataSize(), nullptr, &decoder);
    if (result != MA_SUCCESS)
    {
        return false;
    }
    result = ma_sound_init_from_data_source(&engine, &decoder, MA_SOUND_FLAG_DECODE, nullptr, &sound);
    if (result != MA_SUCCESS)
    {
        return false;
    }
    soundLoaded = true;
    return true;
}

static void openGsnd(const std::string &path)
{
    const Error::ErrorCode errorCode = SoundAsset::CreateFromAsset(path.c_str(), soundAsset);
    if (errorCode != Error::ErrorCode::OK)
    {
        SDKWindow::Get().ErrorMessage(std::format("Failed to open the sound!\n{}", errorCode));
        return;
    }
    if (!loadSound())
    {
        SDKWindow::Get().ErrorMessage(std::format("Failed to load the sound!\n{}", Error::ErrorCode::UNKNOWN));
    }
}

static void importWav(const std::string &path)
{
    const Error::ErrorCode errorCode = SoundAsset::CreateFromWAV(path.c_str(), soundAsset);
    if (errorCode != Error::ErrorCode::OK)
    {
        SDKWindow::Get().ErrorMessage(std::format("Failed to import the sound!\n{}", errorCode));
        return;
    }
    if (!loadSound())
    {
        SDKWindow::Get().ErrorMessage(std::format("Failed to load the sound!\n{}", Error::ErrorCode::UNKNOWN));
    }
}

static void saveGsnd(const std::string &path)
{
    const Error::ErrorCode errorCode = soundAsset.SaveAsAsset(path.c_str());
    if (errorCode != Error::ErrorCode::OK)
    {
        SDKWindow::Get().ErrorMessage(std::format("Failed to save the sound!\n{}", errorCode));
    }
}

static void exportWav(const std::string &path)
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
    constexpr ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDecoration |
                                             ImGuiWindowFlags_NoMove |
                                             ImGuiWindowFlags_NoSavedSettings |
                                             ImGuiWindowFlags_NoBringToFrontOnFocus;
    ImGui::Begin("sndedit", nullptr, windowFlags);
    bool openPressed = ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_O);
    bool importPressed = ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_O);
    bool savePressed = ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_S) && soundLoaded;
    bool exportPressed = ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_S) && soundLoaded;

    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            openPressed |= ImGui::MenuItem("Open", "Ctrl+O");
            importPressed |= ImGui::MenuItem("Import", "Ctrl+Shift+O");
            savePressed |= ImGui::MenuItem("Save", "Ctrl+S", false, soundLoaded);
            exportPressed |= ImGui::MenuItem("Export", "Ctrl+Shift+S", false, soundLoaded);
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
        SDKWindow::Get().OpenFileDialog(openGsnd, DialogFilters::gsndFilters);
    } else if (importPressed)
    {
        SDKWindow::Get().OpenFileDialog(importWav, DialogFilters::wavFilters);
    } else if (savePressed)
    {
        SDKWindow::Get().SaveFileDialog(saveGsnd, DialogFilters::gsndFilters);
    } else if (exportPressed)
    {
        SDKWindow::Get().SaveFileDialog(exportWav, DialogFilters::wavFilters);
    }

    if (soundLoaded)
    {
        float length = 0;
        float cursor = 0;
        const bool isPlaying = ma_sound_is_playing(&sound) == 1;
        bool isLooping = ma_sound_is_looping(&sound) == 1;
        ma_sound_get_length_in_seconds(&sound, &length);
        ma_sound_get_cursor_in_seconds(&sound, &cursor);

        ImGui::SeparatorText("Sound Player");

        ImGui::PushItemWidth(-1);
        float nCursor = cursor;
        if (ImGui::SliderFloat("##Seek", &nCursor, 0.0f, length, ""))
        {
            if (nCursor != cursor)
            {
                ma_sound_seek_to_second(&sound, nCursor);
                if (!isPlaying)
                {
                    ma_sound_start(&sound);
                }
            }
        }

        if (!isPlaying)
        {
            if (ImGui::Button("Play", ImVec2(50, 0)))
            {
                ma_sound_start(&sound);
            }
        } else
        {
            if (ImGui::Button("Pause", ImVec2(50, 0)))
            {
                ma_sound_stop(&sound);
            }
        }

        ImGui::SameLine();
        ImGui::PushItemWidth(100);
        float volume = ma_engine_get_volume(&engine) * 100.0f;
        if (ImGui::SliderFloat("##Volume", &volume, 0.0f, 100.0f, "%.0f%%"))
        {
            ma_engine_set_volume(&engine, volume / 100.0f);
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
            // ReSharper disable once CppRedundantCastExpression
            ma_sound_set_looping(&sound, static_cast<ma_bool32>(isLooping));
        }

        ImGui::SeparatorText("Sound Information");
        ImGui::TextUnformatted(std::format("Size: {} bytes", soundAsset.GetDataSize()).c_str());
        ma_format fmt{};
        ma_uint32 channels = 0;
        ma_uint32 sampleRate = 0;
        ma_uint64 pcmLen = 0;
        ma_decoder_get_data_format(&decoder, &fmt, &channels, &sampleRate, nullptr, 0);
        ma_sound_get_length_in_pcm_frames(&sound, &pcmLen);
        constexpr std::array<const char *, 6> formatNames = {"Unknown", "U8", "S16", "S24", "S32", "F32"};
        ImGui::TextUnformatted(std::format("Format: {}", formatNames.at(fmt)).c_str());
        ImGui::TextUnformatted(std::format("Channels: {}", channels).c_str());
        ImGui::TextUnformatted(std::format("Sample Rate: {} Hz", sampleRate).c_str());
        ImGui::Text("Length: %g:%05.2f", lengthMinutes, lengthSeconds);
        ImGui::TextUnformatted(std::format("Length (PCM frames): {}", pcmLen).c_str());

    } else
    {
        ImGui::TextDisabled("No sound is open. Open or import one from the File menu.");
    }

    ImGui::End();
}

int main(int argc, char **argv)
{
    if (!SDKWindow::Get().Init("GAME SDK Sound Editor"))
    {
        return -1;
    }
    const ma_result res = ma_engine_init(nullptr, &engine);
    if (res != MA_SUCCESS)
    {
        printf("Error: ma_engine_init(): %d\n", res);
        return -1;
    }

    const std::string &openPath = DesktopInterface::Get().GetFileArgument(argc, argv, {".gsnd"});
    if (!openPath.empty())
    {
        openGsnd(openPath);
    } else
    {
        const std::string &importPath = DesktopInterface::Get().GetFileArgument(argc, argv, {".wav"});
        if (!importPath.empty())
        {
            importWav(importPath);
        }
    }

    SDKWindow::Get().MainLoop(Render);

    ma_engine_uninit(&engine);

    SDKWindow::Get().Destroy();

    return 0;
}
