#include <array>
#include <cmath>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>
#include <iostream>
#include <libassets/asset/SoundAsset.h>
#include <miniaudio.h>
#include <SDL3/SDL.h>
#include <unordered_map>
#include "Options.h"
#include "SDLRendererImGuiTextureAssetCache.h"
#include "SharedMgr.h"

SoundAsset soundAsset;
ma_decoder decoder{};
ma_sound sound{};
static bool soundLoaded = false;
SDL_Renderer *renderer = nullptr;
SDL_Window *window;
ma_engine engine{};

constexpr SDL_DialogFileFilter gsndFilter = {"GAME sound (*.gsnd)", "gsnd"};
constexpr SDL_DialogFileFilter wavFilter = {"WAV audio", "wav"};

void destroyExistingSound()
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

bool loadSound()
{
    destroyExistingSound();
    ma_result res = ma_decoder_init_memory(soundAsset.GetData().data(), soundAsset.GetDataSize(), nullptr, &decoder);
    if (res != MA_SUCCESS) return false;
    res = ma_sound_init_from_data_source(&engine, &decoder, MA_SOUND_FLAG_DECODE, nullptr, &sound);
    if (res != MA_SUCCESS) return false;
    soundLoaded = true;
    return true;
}

void openGsndCallback(void * /*userdata*/, const char *const *fileList, int /*filter*/)
{
    if (fileList == nullptr || fileList[0] == nullptr)
    {
        return;
    }
    const Error::ErrorCode e = SoundAsset::CreateFromAsset(fileList[0], soundAsset);
    if (e != Error::ErrorCode::E_OK)
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", std::format("Failed to open the sound!\n{}", Error::ErrorString(e)).c_str(), window);
        return;
    }
    if (!loadSound())
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", std::format("Failed to load the sound!\n{}", Error::ErrorString(Error::ErrorCode::E_UNKNOWN)).c_str(), window);
    }
}

void importCallback(void * /*userdata*/, const char *const *fileList, int /*filter*/)
{
    if (fileList == nullptr || fileList[0] == nullptr)
    {
        return;
    }
    const Error::ErrorCode e = SoundAsset::CreateFromWAV(fileList[0], soundAsset);
    if (e != Error::ErrorCode::E_OK)
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", std::format("Failed to import the sound!\n{}", Error::ErrorString(e)).c_str(), window);
        return;
    }
    if (!loadSound())
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", std::format("Failed to load the sound!\n{}", Error::ErrorString(Error::ErrorCode::E_UNKNOWN)).c_str(), window);
    }
}

void saveGsndCallback(void * /*userdata*/, const char *const *fileList, int /*filter*/)
{
    if (fileList == nullptr || fileList[0] == nullptr)
    {
        return;
    }
    const Error::ErrorCode errorCode = soundAsset.SaveAsAsset(fileList[0]);
    if (errorCode != Error::ErrorCode::E_OK)
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", std::format("Failed to save the sound!\n{}", Error::ErrorString(errorCode)).c_str(), window);
    }
}

void exportCallback(void * /*userdata*/, const char *const *fileList, int /*filter*/)
{
    if (fileList == nullptr || fileList[0] == nullptr)
    {
        return;
    }
    const Error::ErrorCode errorCode = soundAsset.SaveAsWAV(fileList[0]);
    if (errorCode != Error::ErrorCode::E_OK)
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", std::format("Failed to export the sound!\n{}", Error::ErrorString(errorCode)).c_str(), window);
    }
}

static void Render(bool &done, SDL_Window *window)
{
    ImGui::Begin("sndedit",
                 nullptr,
                 ImGuiWindowFlags_NoDecoration |
                         ImGuiWindowFlags_NoMove |
                         ImGuiWindowFlags_NoSavedSettings |
                         ImGuiWindowFlags_NoBringToFrontOnFocus);
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
                done = true;
            }
            ImGui::EndMenu();
        }
        SharedMgr::SharedMenuUI();
        ImGui::EndMainMenuBar();
    }

    if (openPressed)
    {
        SDL_ShowOpenFileDialog(openGsndCallback, nullptr, window, {&gsndFilter}, 1, nullptr, false);
    } else if (importPressed)
    {
        SDL_ShowOpenFileDialog(importCallback, nullptr, window, {&wavFilter}, 1, nullptr, false);
    } else if (savePressed)
    {
        SDL_ShowSaveFileDialog(saveGsndCallback, nullptr, window, {&gsndFilter}, 1, nullptr);
    } else if (exportPressed)
    {
        SDL_ShowSaveFileDialog(exportCallback, nullptr, window, {&wavFilter}, 1, nullptr);
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
            ma_sound_set_looping(&sound, static_cast<ma_bool32>(isLooping));
        }

        ImGui::SeparatorText("Sound Information");
        ImGui::Text("Size: %ld bytes", soundAsset.GetDataSize());
        ma_format fmt;
        ma_uint32 channels;
        ma_uint32 sampleRate;
        ma_uint64 pcmLen;
        ma_decoder_get_data_format(&decoder, &fmt, &channels, &sampleRate, nullptr, 0);
        ma_sound_get_length_in_pcm_frames(&sound, &pcmLen);
        constexpr std::array<const char *, 6> formatNames = {"Unknown", "U8", "S16", "S24", "S32", "F32"};
        ImGui::Text("Format: %s", formatNames[fmt]);
        ImGui::Text("Channels: %d", channels);
        ImGui::Text("Sample Rate: %d Hz", sampleRate);
        ImGui::Text("Length: %g:%05.2f", lengthMinutes, lengthSeconds);
        ImGui::Text("Length (PCM frames): %lld", pcmLen);

    } else
    {
        ImGui::TextDisabled("No sound is open. Open or import one from the File menu.");
    }

    ImGui::End();
}

int main()
{
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD))
    {
        printf("Error: SDL_Init(): %s\n", SDL_GetError());
        return -1;
    }

    const ma_result res = ma_engine_init(nullptr, &engine);
    if (res != MA_SUCCESS)
    {
        printf("Error: ma_engine_init failed: %d\n", res);
        return -1;
    }

    constexpr SDL_WindowFlags windowFlags = SDL_WINDOW_HIDDEN | SDL_WINDOW_RESIZABLE;
    window = SDL_CreateWindow("sndedit", 800, 600, windowFlags);
    if (window == nullptr)
    {
        printf("Error: SDL_CreateWindow(): %s\n", SDL_GetError());
        return -1;
    }
    renderer = SDL_CreateRenderer(window, nullptr);
    SDL_SetRenderVSync(renderer, 1);
    if (renderer == nullptr)
    {
        printf("Error: SDL_CreateRenderer(): %s\n", SDL_GetError());
        return -1;
    }

    SDLRendererImGuiTextureAssetCache cache = SDLRendererImGuiTextureAssetCache(renderer);
    SharedMgr::InitSharedMgr(&cache);

    SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    SDL_ShowWindow(window);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    if (SDL_GetSystemTheme() == SDL_SYSTEM_THEME_DARK)
    {
        ImGui::StyleColorsDark();
    } else
    {
        ImGui::StyleColorsLight();
    }

    ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer3_Init(renderer);

    bool done = false;
    while (!done)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT)
            {
                done = true;
            }
            if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID(window))
            {
                done = true;
            }
        }

        if ((SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED) != 0)
        {
            SDL_Delay(10);
            continue;
        }

        ImGui_ImplSDLRenderer3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        const ImGuiViewport *viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);

        Render(done, window);

        SharedMgr::RenderSharedUI(window);

        ImGui::Render();
        SDL_SetRenderScale(renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
        SDL_SetRenderDrawColorFloat(renderer, 0, 0, 0, 1);
        SDL_RenderClear(renderer);
        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
        SDL_RenderPresent(renderer);
    }

    SharedMgr::DestroySharedMgr();

    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    ma_engine_uninit(&engine);
    SDL_Quit();
    return 0;
}
