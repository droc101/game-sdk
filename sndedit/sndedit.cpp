#include <array>
#include <cmath>
#include <cstdio>
#include <format>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>
#include <libassets/asset/SoundAsset.h>
#include <libassets/util/Error.h>
#include <miniaudio.h>
#include <SDL3/SDL_dialog.h>
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_messagebox.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_timer.h>
#include <SDL3/SDL_video.h>
#include "DialogFilters.h"
#include "SDLRendererImGuiTextureAssetCache.h"
#include "SharedMgr.h"

static SoundAsset soundAsset{};
static ma_decoder decoder{};
static ma_sound sound{};
static bool soundLoaded = false;
static SDL_Renderer *renderer{};
static SDL_Window *window{};
static ma_engine engine{};

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

static void openGsndCallback(void * /*userdata*/, const char *const *fileList, int /*filter*/)
{
    if (fileList == nullptr || fileList[0] == nullptr)
    {
        return;
    }
    const Error::ErrorCode errorCode = SoundAsset::CreateFromAsset(fileList[0], soundAsset);
    if (errorCode != Error::ErrorCode::OK)
    {
        if (!SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
                                      "Error",
                                      std::format("Failed to open the sound!\n{}", errorCode).c_str(),
                                      window))
        {
            printf("Error: SDL_ShowSimpleMessageBox(): %s\n", SDL_GetError());
        }
        return;
    }
    if (!loadSound())
    {
        if (!SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
                                      "Error",
                                      std::format("Failed to load the sound!\n{}", Error::ErrorCode::UNKNOWN).c_str(),
                                      window))
        {
            printf("Error: SDL_ShowSimpleMessageBox(): %s\n", SDL_GetError());
        }
    }
}

static void importCallback(void * /*userdata*/, const char *const *fileList, int /*filter*/)
{
    if (fileList == nullptr || fileList[0] == nullptr)
    {
        return;
    }
    const Error::ErrorCode errorCode = SoundAsset::CreateFromWAV(fileList[0], soundAsset);
    if (errorCode != Error::ErrorCode::OK)
    {
        if (!SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
                                      "Error",
                                      std::format("Failed to import the sound!\n{}", errorCode).c_str(),
                                      window))
        {
            printf("Error: SDL_ShowSimpleMessageBox(): %s\n", SDL_GetError());
        }
        return;
    }
    if (!loadSound())
    {
        if (!SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
                                      "Error",
                                      std::format("Failed to load the sound!\n{}", Error::ErrorCode::UNKNOWN).c_str(),
                                      window))
        {
            printf("Error: SDL_ShowSimpleMessageBox(): %s\n", SDL_GetError());
        }
    }
}

static void saveGsndCallback(void * /*userdata*/, const char *const *fileList, int /*filter*/)
{
    if (fileList == nullptr || fileList[0] == nullptr)
    {
        return;
    }
    const Error::ErrorCode errorCode = soundAsset.SaveAsAsset(fileList[0]);
    if (errorCode != Error::ErrorCode::OK)
    {
        if (!SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
                                      "Error",
                                      std::format("Failed to save the sound!\n{}", errorCode).c_str(),
                                      window))
        {
            printf("Error: SDL_ShowSimpleMessageBox(): %s\n", SDL_GetError());
        }
    }
}

static void exportCallback(void * /*userdata*/, const char *const *fileList, int /*filter*/)
{
    if (fileList == nullptr || fileList[0] == nullptr)
    {
        return;
    }
    const Error::ErrorCode errorCode = soundAsset.SaveAsWAV(fileList[0]);
    if (errorCode != Error::ErrorCode::OK)
    {
        if (!SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
                                      "Error",
                                      std::format("Failed to export the sound!\n{}", errorCode).c_str(),
                                      window))
        {
            printf("Error: SDL_ShowSimpleMessageBox(): %s\n", SDL_GetError());
        }
    }
}

static void Render(bool &done)
{
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
                done = true;
            }
            ImGui::EndMenu();
        }
        SharedMgr::SharedMenuUI("sndedit");
        ImGui::EndMainMenuBar();
    }

    if (openPressed)
    {
        SDL_ShowOpenFileDialog(openGsndCallback,
                               nullptr,
                               sdlWindow,
                               DialogFilters::gsndFilters.data(),
                               1,
                               nullptr,
                               false);
    } else if (importPressed)
    {
        SDL_ShowOpenFileDialog(importCallback, nullptr, sdlWindow, DialogFilters::wavFilters.data(), 1, nullptr, false);
    } else if (savePressed)
    {
        SDL_ShowSaveFileDialog(saveGsndCallback, nullptr, sdlWindow, DialogFilters::gsndFilters.data(), 1, nullptr);
    } else if (exportPressed)
    {
        SDL_ShowSaveFileDialog(exportCallback, nullptr, sdlWindow, DialogFilters::wavFilters.data(), 1, nullptr);
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

int main()
{
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        printf("Error: SDL_Init(): %s\n", SDL_GetError());
        return -1;
    }

    const ma_result res = ma_engine_init(nullptr, &engine);
    if (res != MA_SUCCESS)
    {
        printf("Error: ma_engine_init(): %d\n", res);
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
    if (!SDL_SetRenderVSync(renderer, 1))
    {
        printf("Error: SDL_SetRenderVSync(): %s\n", SDL_GetError());
    }
    if (renderer == nullptr)
    {
        printf("Error: SDL_CreateRenderer(): %s\n", SDL_GetError());
        return -1;
    }

    SharedMgr::InitSharedMgr<SDLRendererImGuiTextureAssetCache>(renderer);

    if (!SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED))
    {
        printf("Error: SDL_SetWindowPosition(): %s\n", SDL_GetError());
    }
    if (!SDL_ShowWindow(window))
    {
        printf("Error: SDL_ShowWindow(): %s\n", SDL_GetError());
        return -1;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    SharedMgr::ApplyTheme();

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

        Render(done);

        SharedMgr::RenderSharedUI(window);

        ImGui::Render();
        if (!SDL_SetRenderScale(renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y))
        {
            printf("Error: SDL_SetRenderScale(): %s\n", SDL_GetError());
        }
        if (!SDL_SetRenderDrawColorFloat(renderer, 0, 0, 0, 1))
        {
            printf("Error: SDL_SetRenderDrawColorFloat(): %s\n", SDL_GetError());
        }
        if (!SDL_RenderClear(renderer))
        {
            printf("Error: SDL_RenderClear(): %s\n", SDL_GetError());
        }
        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
        if (!SDL_RenderPresent(renderer))
        {
            printf("Error: SDL_RenderPresent(): %s\n", SDL_GetError());
        }
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
