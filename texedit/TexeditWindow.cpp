//
// Created by droc101 on 8/21/26.
//

#include "TexeditWindow.h"
#include <cassert>
#include <format>
#include <game_sdk/DialogFilters.h>
#include <game_sdk/SharedMgr.h>
#include <game_sdk/Window.h>
#include <imgui.h>
#include <libassets/util/Error.h>
#include <utility>

const Window::WindowProperties &TexeditWindow::GetProperties() const
{
    return properties;
}

bool TexeditWindow::Init()
{
    (void)SharedMgr::Get().textureCache.RegisterPng("assets/icons/checkerboard.png",
                                                    CHECKERBOARD_ICON_NAME,
                                                    false,
                                                    true);
    const Error::ErrorCode e = SharedMgr::Get().textureCache.GetTextureID(CHECKERBOARD_ICON_NAME, checkerboardTexture);
    assert(e == Error::ErrorCode::OK);

    // const std::string &openPath = DesktopInterface::Get().GetFileArgument(argc, argv, {".gtex"});
    // if (!openPath.empty())
    // {
    //     OpenGtex(openPath);
    // } else
    // {
    //     const std::string &importPath = DesktopInterface::Get().GetFileArgument(argc,
    //                                                                             argv,
    //                                                                             {
    //                                                                                 ".png",
    //                                                                                 ".exr",
    //                                                                             });
    //     if (!importPath.empty())
    //     {
    //         ImportImage(importPath);
    //     }
    // }
    return true;
}

void TexeditWindow::Destroy()
{
    DestroyExistingTexture();
}

void TexeditWindow::DestroyExistingTexture()
{
    if (!textureLoaded)
    {
        return;
    }
    glDeleteTextures(1, &glTexture);
    textureLoaded = false;
}

void TexeditWindow::LoadTexture()
{
    DestroyExistingTexture();

    glTexture = GLTextureCache::CreateTexture(texture);
    textureLoaded = true;
}

void TexeditWindow::OpenGtex(const std::string &path)
{
    const Error::ErrorCode errorCode = texture.LoadFromAsset(path);
    if (errorCode != Error::ErrorCode::OK)
    {
        ErrorMessage(std::format("Failed to open the texture!\n{}", errorCode));
        return;
    }
    LoadTexture();
}

void TexeditWindow::ImportImage(const std::string &path)
{
    Error::ErrorCode errorCode = texture.Import(path);
    if (errorCode != Error::ErrorCode::OK)
    {
        ErrorMessage(std::format("Failed to import the texture!\n{}", errorCode));
        return;
    }
    LoadTexture();
}

void TexeditWindow::SaveGtex(const std::string &path)
{
    const Error::ErrorCode errorCode = texture.SaveToAsset(path);
    if (errorCode != Error::ErrorCode::OK)
    {
        ErrorMessage(std::format("Failed to save the texture!\n{}", errorCode));
    }
}

void TexeditWindow::Export(const std::string &path)
{
    const Error::ErrorCode errorCode = texture.Export(path);
    if (errorCode != Error::ErrorCode::OK)
    {
        ErrorMessage(std::format("Failed to export the texture!\n{}", errorCode));
    }
}

void TexeditWindow::ClampZoom()
{
    if (zoom > MAX_ZOOM)
    {
        zoom = MAX_ZOOM;
    }
    if (zoom < MIN_ZOOM)
    {
        zoom = MIN_ZOOM;
    }
}

void TexeditWindow::Render()
{
    bool openPressed = ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_O);
    bool importPressed = ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_O);
    bool savePressed = ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_S) && textureLoaded;
    bool exportPressed = ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_S) && textureLoaded;

    bool zoomInPressed = ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_Equal, ImGuiInputFlags_Repeat) && textureLoaded;
    bool zoomOutPressed = ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_Minus, ImGuiInputFlags_Repeat) && textureLoaded;
    bool resetZoomPressed = ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_0) && textureLoaded;

    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            openPressed |= ImGui::MenuItem("Open", "Ctrl+O");
            importPressed |= ImGui::MenuItem("Import", "Ctrl+Shift+O");
            savePressed |= ImGui::MenuItem("Save", "Ctrl+S", false, textureLoaded);
            exportPressed |= ImGui::MenuItem("Export", "Ctrl+Shift+S", false, textureLoaded);
            ImGui::Separator();
            if (ImGui::MenuItem("Quit", "Alt+F4"))
            {
                closeRequest = true;
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View", textureLoaded))
        {
            zoomInPressed |= ImGui::MenuItem("Zoom In", "Ctrl+=");
            zoomOutPressed |= ImGui::MenuItem("Zoom Out", "Ctrl+-");
            resetZoomPressed |= ImGui::MenuItem("Reset Zoom", "Ctrl+0");
            ImGui::Separator();
            if (ImGui::MenuItem("Center Image"))
            {
                pan = {0, 0};
            }
            ImGui::Separator();
            ImGui::MenuItem("Transparency Checkerboard", "", &showTransparencyCheckerboard);
            ImGui::EndMenu();
        }
        SharedMgr::Get().SharedMenuUI("texedit");
        ImGui::EndMainMenuBar();
    }

    if (openPressed)
    {
        OpenFileDialog(FILE_DIALOG_CALLBACK(OpenGtex), DialogFilters::GTEX_FILTERS);
    } else if (importPressed)
    {
        OpenFileDialog(FILE_DIALOG_CALLBACK(ImportImage), DialogFilters::IMAGE_FILTERS);
    } else if (savePressed)
    {
        SaveFileDialog(FILE_DIALOG_CALLBACK(SaveGtex), DialogFilters::GTEX_FILTERS);
    } else if (exportPressed)
    {
        FileDialogCallback exportFn = FILE_DIALOG_CALLBACK(Export);
        if (texture.GetFormat() == TextureAsset::PixelFormat::RGBA8)
        {
            SaveFileDialog(std::move(exportFn), DialogFilters::PNG_FILTERS);
        } else
        {
            SaveFileDialog(std::move(exportFn), DialogFilters::EXR_FILTERS);
        }
    } else if (zoomInPressed)
    {
        zoom += 0.1;
        ClampZoom();
    } else if (zoomOutPressed)
    {
        zoom -= 0.1;
        ClampZoom();
    } else if (resetZoomPressed)
    {
        zoom = 1.0f;
    }

    if (textureLoaded)
    {
        zoom += ImGui::GetIO().MouseWheel * 0.1f;
        ClampZoom();

        if (ImGui::IsMouseDragging(ImGuiMouseButton_Right))
        {
            const ImVec2 dragDelta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Right);
            pan = {pan.x + (dragDelta.x), pan.y + (dragDelta.y)};
            ImGui::ResetMouseDragDelta(ImGuiMouseButton_Right);
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
        }

        const ImVec2 &availableSize = ImGui::GetContentRegionAvail();

        constexpr float STATS_WIDTH = 150.0f;
        const float imageWidth = availableSize.x - STATS_WIDTH - 8.0f;

        ImGui::BeginChild("ImagePane",
                          ImVec2(imageWidth, availableSize.y),
                          ImGuiChildFlags_Borders,
                          ImGuiWindowFlags_NoScrollbar |
                                  ImGuiWindowFlags_NoBringToFrontOnFocus |
                                  ImGuiWindowFlags_NoScrollWithMouse);
        {
            const ImVec2 imageSize{static_cast<float>(texture.GetWidth()) * zoom,
                                   static_cast<float>(texture.GetHeight()) * zoom};
            const ImVec2 panelSize = ImGui::GetContentRegionAvail();
            const ImVec2 cursor = {((panelSize.x / 2) - (imageSize.x / 2)) + pan.x,
                                   ((panelSize.y / 2) - (imageSize.y / 2)) + pan.y};
            ImGui::SetCursorPos(cursor);
            if (showTransparencyCheckerboard)
            {
                const ImVec2 checkerboardUv = {imageSize.x / 16.0f, imageSize.y / 16.0f};
                ImGui::Image(checkerboardTexture, imageSize, {0, 0}, checkerboardUv);
                ImGui::SetCursorPos(cursor);
            }
            ImGui::Image(glTexture, imageSize);
        }
        ImGui::EndChild();
        ImGui::SameLine();

        ImGui::BeginChild("StatsPane", ImVec2(STATS_WIDTH, availableSize.y));
        {
            ImGui::TextUnformatted(std::format("Width: {}px\nHeight: {}px\nMemory: {} bytes\nFormat: {}",
                                               texture.GetWidth(),
                                               texture.GetHeight(),
                                               texture.GetPixelDataSize(),
                                               texture.GetFormat() == TextureAsset::PixelFormat::RGBA8
                                                       ? "RGBA8 (SDR)"
                                                       : "RGBA16F (HDR)")
                                           .c_str());

            ImGui::Separator();
            if (ImGui::Checkbox("Filter", &texture.filter))
            {
                LoadTexture();
            }
            if (ImGui::Checkbox("Repeat", &texture.repeat))
            {
                LoadTexture();
            }
            if (ImGui::Checkbox("Mipmaps", &texture.mipmaps))
            {
                LoadTexture();
            }
        }
        ImGui::EndChild();

    } else
    {
        ImGui::TextDisabled("No texture is open. Open or import one from the File menu.");
    }
}
