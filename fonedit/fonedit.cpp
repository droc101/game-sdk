//
// Created by droc101 on 7/23/25.
//
//
// Created by droc101 on 7/23/25.
//
#include <array>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>
#include <iostream>
#include <libassets/asset/TextureAsset.h>
#include <misc/cpp/imgui_stdlib.h>
#include <SDL3/SDL.h>
#include "libassets/asset/FontAsset.h"
#include "SDLRendererImGuiTextureAssetCache.h"
#include "SharedMgr.h"
#include "TextureBrowserWindow.h"

static FontAsset font;
static bool fontLoaded = false;
SDL_Renderer *renderer;
SDL_Window *window;
std::vector<std::string> charDisplayList;

constexpr SDL_DialogFileFilter gfonFilter = {"GAME font (*.gfon)", "gfon"};

void openGfonCallback(void * /*userdata*/, const char *const *fileList, int /*filter*/)
{
    if (fileList == nullptr || fileList[0] == nullptr)
    {
        return;
    }
    const Error::ErrorCode e = FontAsset::CreateFromAsset(fileList[0], font);
    if (e != Error::ErrorCode::E_OK)
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", std::format("Failed to open the font!\n{}", Error::ErrorString(e)).c_str(), window);
        return;
    }
    fontLoaded = true;
}

void saveGfonCallback(void * /*userdata*/, const char *const *fileList, int /*filter*/)
{
    if (fileList == nullptr || fileList[0] == nullptr)
    {
        return;
    }
    const Error::ErrorCode errorCode = font.SaveAsAsset(fileList[0]);
    if (errorCode != Error::ErrorCode::E_OK)
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", std::format("Failed to save the font!\n{}", Error::ErrorString(errorCode)).c_str(), window);
    }
}

bool ComboGetter(void* data, const int idx, const char** out_text) {
    const std::vector<std::string> &items = *static_cast<std::vector<std::string>*>(data);
    if (idx < 0 || idx >= static_cast<int>(items.size())) return false;
    *out_text = items[idx].c_str();
    return true;
}

template <typename T> void MoveBack(std::vector<T>& vec, size_t index)
{
    if (index > 0) {
        std::iter_swap(vec.begin() + index, vec.begin() + index - 1);
    }
}

template <typename T> void MoveForward(std::vector<T>& vec, size_t index) {
    if (index + 1 < vec.size()) {
        std::iter_swap(vec.begin() + index, vec.begin() + index + 1);
    }
}


static void Render(bool &done, SDL_Window *window)
{
    ImGui::Begin("fonedit",
                 nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
                 ImGuiWindowFlags_NoBringToFrontOnFocus);
    bool newPressed = ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_N);
    bool openPressed = ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_O);
    bool savePressed = ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_S) && fontLoaded;

    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            newPressed |= ImGui::MenuItem("New", "Ctrl+N");
            ImGui::Separator();
            openPressed |= ImGui::MenuItem("Open", "Ctrl+O");
            savePressed |= ImGui::MenuItem("Save", "Ctrl+S", false, fontLoaded);
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
        SDL_ShowOpenFileDialog(openGfonCallback, nullptr, window, {&gfonFilter}, 1, nullptr, false);
    } else if (savePressed)
    {
        SDL_ShowSaveFileDialog(saveGfonCallback, nullptr, window, {&gfonFilter}, 1, nullptr);
    } else if (newPressed)
    {
        font = FontAsset();
        fontLoaded = true;
    }

    if (fontLoaded)
    {
        const ImVec2 &availableSize = ImGui::GetContentRegionAvail();

        constexpr float statsWidth = 300.0f;
        const float imageWidth = availableSize.x - statsWidth - 8.0f;

        ImGui::BeginChild("ImagePane",
                          ImVec2(imageWidth, availableSize.y),
                          ImGuiChildFlags_Border,
                          ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoBringToFrontOnFocus);
        {
            if (ImGui::Button("Add Symbol"))
            {
                if (!font.chars.empty())
                {
                    font.chars.push_back(static_cast<char>(font.chars.back() + 1));
                } else
                {
                    font.chars.push_back('a');
                }

                font.char_widths.push_back(font.defaultSize);
            }
            const ImVec2 availSize = ImGui::GetContentRegionAvail();
            if (ImGui::BeginTable("charTable", 4 , ImGuiTableFlags_ScrollY, availSize))
            {
                ImGui::TableSetupColumn("Slot", ImGuiTableColumnFlags_NoSort | ImGuiTableColumnFlags_WidthFixed, 40);
                ImGui::TableSetupColumn("Symbol", ImGuiTableColumnFlags_NoSort | ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("Display Width", ImGuiTableColumnFlags_NoSort | ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("Controls", ImGuiTableColumnFlags_NoSort | ImGuiTableColumnFlags_WidthFixed, (40 * 3) + (ImGui::GetStyle().WindowPadding.x * 3));
                ImGui::TableHeadersRow();
                for (size_t i = 0; i < font.chars.size(); i++)
                {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text(std::format("{}", i).c_str());
                    ImGui::TableNextColumn();
                    int char_index = static_cast<int>(std::string(FontAsset::FONT_VALID_CHARS).find(font.chars.at(i)));
                    ImGui::PushItemWidth(-1);
                    if (ImGui::Combo(std::format("##Char_{}", i).c_str(), &char_index, ComboGetter, &charDisplayList, static_cast<int>(charDisplayList.size())))
                    {
                        font.chars.at(i) = FontAsset::FONT_VALID_CHARS[char_index];
                    }
                    ImGui::TableNextColumn();
                    int width = font.char_widths.at(i);
                    ImGui::PushItemWidth(-1);
                    if (ImGui::SliderInt(std::format("##CharWidth_{}", i).c_str(), &width, 1, 32, "%d px"))
                    {
                        font.char_widths.at(i) = width;
                    }
                    ImGui::TableNextColumn();
                    if (ImGui::Button(std::format("Up##{}", i).c_str(), ImVec2(40, 0)))
                    {
                        MoveBack<uint8_t>(font.char_widths, i);
                        MoveBack<char>(font.chars, i);
                    }
                    ImGui::SameLine();
                    if (ImGui::Button(std::format("Down##{}", i).c_str(), ImVec2(40, 0)))
                    {
                        MoveForward<uint8_t>(font.char_widths, i);
                        MoveForward<char>(font.chars, i);
                    }
                    ImGui::SameLine();
                    if (ImGui::Button(std::format("Del##{}", i).c_str(), ImVec2(40, 0)))
                    {
                        font.char_widths.erase(font.char_widths.begin() + static_cast<ptrdiff_t>(i));
                        font.chars.erase(font.chars.begin() + static_cast<ptrdiff_t>(i));
                    }
                }
                ImGui::EndTable();
            }
        }
        ImGui::EndChild();
        ImGui::SameLine();

        ImGui::BeginChild("StatsPane", ImVec2(statsWidth, availableSize.y));
        {
            int charWidth = font.charWidth;
            int textureHeight = font.textureHeight;
            int baseline = font.baseline;
            int charSpacing = font.charSpacing;
            int lineSpacing = font.lineSpacing;
            int spaceWidth = font.spaceWidth;
            int defaultSize = font.defaultSize;

            ImGui::PushItemWidth(-1);
            ImGui::Text("Char slot width in texture");
            if (ImGui::SliderInt("##charWidth", &charWidth, 1, 32))
            {
                font.charWidth = charWidth;
            }
            ImGui::Text("Texture Height");
            if (ImGui::SliderInt("##textureHeight", &textureHeight, 1, 32))
            {
                font.textureHeight = textureHeight;
            }
            ImGui::Text("Baseline");
            if (ImGui::SliderInt("##baseline", &baseline, 1, 32))
            {
                font.baseline = baseline;
            }
            ImGui::Text("Symbol Spacing");
            if (ImGui::SliderInt("##charSpacing", &charSpacing, 0, 8))
            {
                font.charSpacing = charSpacing;
            }
            ImGui::Text("Line Spacing");
            if (ImGui::SliderInt("##lineSpacing", &lineSpacing, 0, 8))
            {
                font.lineSpacing = lineSpacing;
            }
            ImGui::Text("Space Width");
            if (ImGui::SliderInt("##spaceWidth", &spaceWidth, 0, 32))
            {
                font.spaceWidth = spaceWidth;
            }
            ImGui::Text("Default Font Size");
            if (ImGui::SliderInt("##defaultSize", &defaultSize, 0, 32))
            {
                font.defaultSize = defaultSize;
            }
            ImGui::Text("Font Texture");
            TextureBrowserWindow::InputTexture("##Texture", font.texture);
            ImGui::Checkbox("Uppercase only", &font.uppercaseOnly);
        }
        ImGui::EndChild();
    } else
    {
        ImGui::TextDisabled("No font is open. Open or create one from the File menu.");
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

    constexpr SDL_WindowFlags windowFlags = SDL_WINDOW_HIDDEN | SDL_WINDOW_RESIZABLE;
    window = SDL_CreateWindow("fonedit", 800, 600, windowFlags);
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

    charDisplayList = FontAsset::GetCharListForDisplay();

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
    SDL_Quit();
    return 0;
}
