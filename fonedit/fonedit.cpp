//
// Created by droc101 on 7/23/25.
//
//
// Created by droc101 on 7/23/25.
//

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <format>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>
#include <libassets/asset/FontAsset.h>
#include <libassets/util/Error.h>
#include <libassets/util/VectorMove.h>
#include <SDL3/SDL_dialog.h>
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_messagebox.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_timer.h>
#include <SDL3/SDL_video.h>
#include <string>
#include <vector>
#include "DialogFilters.h"
#include "SDLRendererImGuiTextureAssetCache.h"
#include "SharedMgr.h"
#include "TextureBrowserWindow.h"

static FontAsset font{};
static bool fontLoaded = false;
static SDL_Renderer *renderer = nullptr;
static SDL_Window *window = nullptr;
static std::vector<std::string> charDisplayList{};

static void openGfon(const std::string &path)
{
    const Error::ErrorCode errorCode = FontAsset::CreateFromAsset(path.c_str(), font);
    if (errorCode != Error::ErrorCode::OK)
    {
        if (!SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
                                      "Error",
                                      std::format("Failed to open the font!\n{}", errorCode).c_str(),
                                      window))
        {
            printf("Failed to show SDL Error messagebox with error \"%s\"\n", SDL_GetError());
        }
        return;
    }
    fontLoaded = true;
}

static void openGfonCallback(void * /*userdata*/, const char *const *fileList, int /*filter*/)
{
    if (fileList == nullptr || fileList[0] == nullptr)
    {
        return;
    }
    openGfon(fileList[0]);
}

static void saveGfonCallback(void * /*userdata*/, const char *const *fileList, int /*filter*/)
{
    if (fileList == nullptr || fileList[0] == nullptr)
    {
        return;
    }
    const Error::ErrorCode errorCode = font.SaveAsAsset(fileList[0]);
    if (errorCode != Error::ErrorCode::OK)
    {
        if (!SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
                                      "Error",
                                      std::format("Failed to save the font!\n{}", errorCode).c_str(),
                                      window))
        {
            printf("Failed to show SDL Error messagebox with error \"%s\"\n", SDL_GetError());
        }
    }
}

static bool ComboGetter(void *data, const int index, const char **out_text)
{
    const std::vector<std::string> &items = *static_cast<std::vector<std::string> *>(data);
    if (index < 0 || static_cast<size_t>(index) >= items.size())
    {
        return false;
    }
    *out_text = items[index].c_str();
    return true;
}

static void Render(bool &done, SDL_Window *sdlWindow)
{
    constexpr ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDecoration |
                                             ImGuiWindowFlags_NoMove |
                                             ImGuiWindowFlags_NoSavedSettings |
                                             ImGuiWindowFlags_NoBringToFrontOnFocus;
    ImGui::Begin("fonedit", nullptr, windowFlags);
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
        SharedMgr::SharedMenuUI("fonedit");
        ImGui::EndMainMenuBar();
    }

    if (openPressed)
    {
        SDL_ShowOpenFileDialog(openGfonCallback,
                               nullptr,
                               sdlWindow,
                               DialogFilters::gfonFilters.data(),
                               1,
                               nullptr,
                               false);
    } else if (savePressed)
    {
        SDL_ShowSaveFileDialog(saveGfonCallback, nullptr, sdlWindow, DialogFilters::gfonFilters.data(), 1, nullptr);
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
                          ImGuiChildFlags_Borders,
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

                font.charWidths.push_back(font.defaultSize);
            }
            const ImVec2 availSize = ImGui::GetContentRegionAvail();
            if (ImGui::BeginTable("charTable", 4, ImGuiTableFlags_ScrollY, availSize))
            {
                ImGui::TableSetupColumn("Slot", ImGuiTableColumnFlags_NoSort | ImGuiTableColumnFlags_WidthFixed, 40);
                ImGui::TableSetupColumn("Symbol", ImGuiTableColumnFlags_NoSort | ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("Display Width",
                                        ImGuiTableColumnFlags_NoSort | ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("Controls",
                                        ImGuiTableColumnFlags_NoSort | ImGuiTableColumnFlags_WidthFixed,
                                        (40 * 3) + (ImGui::GetStyle().WindowPadding.x * 3));
                ImGui::TableHeadersRow();
                for (size_t i = 0; i < font.chars.size(); i++)
                {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextUnformatted(std::format("{}", i).c_str());
                    ImGui::TableNextColumn();
                    int charIndex = static_cast<int>(FontAsset::FONT_VALID_CHARS.find(font.chars.at(i)));
                    ImGui::PushItemWidth(-1);
                    if (ImGui::Combo(std::format("##Char_{}", i).c_str(),
                                     &charIndex,
                                     ComboGetter,
                                     &charDisplayList,
                                     static_cast<int>(charDisplayList.size())))
                    {
                        font.chars.at(i) = FontAsset::FONT_VALID_CHARS[charIndex];
                    }
                    ImGui::TableNextColumn();
                    int width = font.charWidths.at(i);
                    ImGui::PushItemWidth(-1);
                    if (ImGui::SliderInt(std::format("##CharWidth_{}", i).c_str(), &width, 1, 32, "%d px"))
                    {
                        font.charWidths.at(i) = width;
                    }
                    ImGui::TableNextColumn();
                    if (ImGui::Button(std::format("Up##{}", i).c_str(), ImVec2(40, 0)))
                    {
                        MoveBack<uint8_t>(font.charWidths, i);
                        MoveBack<char>(font.chars, i);
                    }
                    ImGui::SameLine();
                    if (ImGui::Button(std::format("Down##{}", i).c_str(), ImVec2(40, 0)))
                    {
                        MoveForward<uint8_t>(font.charWidths, i);
                        MoveForward<char>(font.chars, i);
                    }
                    ImGui::SameLine();
                    if (ImGui::Button(std::format("Del##{}", i).c_str(), ImVec2(40, 0)))
                    {
                        font.charWidths.erase(font.charWidths.begin() + static_cast<ptrdiff_t>(i));
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
            ImGui::TextUnformatted("Char slot width in texture");
            if (ImGui::SliderInt("##charWidth", &charWidth, 1, 32))
            {
                font.charWidth = charWidth;
            }
            ImGui::TextUnformatted("Texture Height");
            if (ImGui::SliderInt("##textureHeight", &textureHeight, 1, 32))
            {
                font.textureHeight = textureHeight;
            }
            ImGui::TextUnformatted("Baseline");
            if (ImGui::SliderInt("##baseline", &baseline, 1, 32))
            {
                font.baseline = baseline;
            }
            ImGui::TextUnformatted("Symbol Spacing");
            if (ImGui::SliderInt("##charSpacing", &charSpacing, 0, 8))
            {
                font.charSpacing = charSpacing;
            }
            ImGui::TextUnformatted("Line Spacing");
            if (ImGui::SliderInt("##lineSpacing", &lineSpacing, 0, 8))
            {
                font.lineSpacing = lineSpacing;
            }
            ImGui::TextUnformatted("Space Width");
            if (ImGui::SliderInt("##spaceWidth", &spaceWidth, 0, 32))
            {
                font.spaceWidth = spaceWidth;
            }
            ImGui::TextUnformatted("Default Font Size");
            if (ImGui::SliderInt("##defaultSize", &defaultSize, 0, 32))
            {
                font.defaultSize = defaultSize;
            }
            ImGui::TextUnformatted("Font Texture");
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

int main(int argc, char **argv)
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

    charDisplayList = FontAsset::GetCharListForDisplay();

    const std::string &openPath = DesktopInterface::GetFileArgument(argc, argv, {".gfon"});
    if (!openPath.empty())
    {
        openGfon(openPath);
    }

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
    SDL_Quit();
    return 0;
}
