#include <cassert>
#include <cstdint>
#include <format>
#include <game_sdk/DesktopInterface.h>
#include <game_sdk/DialogFilters.h>
#include <game_sdk/SDKWindow.h>
#include <game_sdk/SharedMgr.h>
#include <GL/glew.h>
#include <imgui.h>
#include <libassets/asset/TextureAsset.h>
#include <libassets/util/Error.h>
#include <string>
#include <vector>

static float zoom = 1.0f;
static ImVec2 pan = {0, 0};

static TextureAsset texture{};
static bool textureLoaded = false;
static GLuint glTexture;

static bool showTransparencyCheckerboard = true;

constexpr const char *CHECKERBOARD_ICON_NAME = "editor/checkerboard";
static ImTextureID checkerboardTexture;

constexpr float MIN_ZOOM = 0.1f;
constexpr float MAX_ZOOM = 10.0f;

static void destroyExistingTexture()
{
    if (!textureLoaded)
    {
        return;
    }
    glDeleteTextures(1, &glTexture);
    textureLoaded = false;
}

static void loadTexture()
{
    destroyExistingTexture();

    glTexture = GLTextureCache::CreateTexture(texture);
    textureLoaded = true;
}

static void openGtex(const std::string &path)
{
    const Error::ErrorCode errorCode = TextureAsset::CreateFromAsset(path.c_str(), texture);
    if (errorCode != Error::ErrorCode::OK)
    {
        SDKWindow::Get().ErrorMessage(std::format("Failed to open the texture!\n{}", errorCode));
        return;
    }
    loadTexture();
}

static void importImage(const std::string &path)
{
    Error::ErrorCode errorCode = Error::ErrorCode::INCORRECT_FORMAT;
    if (path.ends_with(".png"))
    {
        errorCode = TextureAsset::CreateFromPNG(path.c_str(), texture);
    } else
    {
        errorCode = TextureAsset::CreateFromEXR(path.c_str(), texture);
    }
    if (errorCode != Error::ErrorCode::OK)
    {
        SDKWindow::Get().ErrorMessage(std::format("Failed to import the texture!\n{}", errorCode));
        return;
    }
    loadTexture();
}

static void saveGtex(const std::string &path)
{
    const Error::ErrorCode errorCode = texture.SaveAsAsset(path.c_str());
    if (errorCode != Error::ErrorCode::OK)
    {
        SDKWindow::Get().ErrorMessage(std::format("Failed to save the texture!\n{}", errorCode));
    }
}

static void exportPng(const std::string &path)
{
    const Error::ErrorCode errorCode = texture.SaveAsPNG(path.c_str());
    if (errorCode != Error::ErrorCode::OK)
    {
        SDKWindow::Get().ErrorMessage(std::format("Failed to export the texture!\n{}", errorCode));
    }
}

static void exportExr(const std::string &path)
{
    const Error::ErrorCode errorCode = texture.SaveAsEXR(path.c_str());
    if (errorCode != Error::ErrorCode::OK)
    {
        SDKWindow::Get().ErrorMessage(std::format("Failed to export the texture!\n{}", errorCode));
    }
}

static void ClampZoom()
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

static void Render()
{
    const ImGuiViewport *viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    constexpr ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDecoration |
                                             ImGuiWindowFlags_NoMove |
                                             ImGuiWindowFlags_NoSavedSettings |
                                             ImGuiWindowFlags_NoBringToFrontOnFocus;
    ImGui::Begin("texedit", nullptr, windowFlags);
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
                SDKWindow::Get().PostQuit();
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
        SDKWindow::Get().OpenFileDialog(openGtex, DialogFilters::gtexFilters);
    } else if (importPressed)
    {
        SDKWindow::Get().OpenFileDialog(importImage, DialogFilters::imageFilters);
    } else if (savePressed)
    {
        SDKWindow::Get().SaveFileDialog(saveGtex, DialogFilters::gtexFilters);
    } else if (exportPressed)
    {
        if (texture.GetFormat() == TextureAsset::PixelFormat::RGBA8)
        {
            SDKWindow::Get().SaveFileDialog(exportPng, DialogFilters::pngFilters);
        } else
        {
            SDKWindow::Get().SaveFileDialog(exportExr, DialogFilters::exrFilters);
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
                          ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoScrollWithMouse);
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
                loadTexture();
            }
            if (ImGui::Checkbox("Repeat", &texture.repeat))
            {
                loadTexture();
            }
            if (ImGui::Checkbox("Mipmaps", &texture.mipmaps))
            {
                loadTexture();
            }
        }
        ImGui::EndChild();

    } else
    {
        ImGui::TextDisabled("No texture is open. Open or import one from the File menu.");
    }

    ImGui::End();
}

int main(int argc, char **argv)
{
    if (!SDKWindow::Get().Init("GAME SDK Texture Editor"))
    {
        return -1;
    }

    SDKWindow::Get().SetWindowIcon("texedit");

    (void)SharedMgr::Get().textureCache.RegisterPng("assets/icons/checkerboard.png",
                                                    CHECKERBOARD_ICON_NAME,
                                                    false,
                                                    true);
    const Error::ErrorCode e = SharedMgr::Get().textureCache.GetTextureID(CHECKERBOARD_ICON_NAME, checkerboardTexture);
    assert(e == Error::ErrorCode::OK);

    const std::string &openPath = DesktopInterface::Get().GetFileArgument(argc, argv, {".gtex"});
    if (!openPath.empty())
    {
        openGtex(openPath);
    } else
    {
        const std::string &importPath = DesktopInterface::Get().GetFileArgument(argc,
                                                                                argv,
                                                                                {
                                                                                    ".png",
                                                                                    ".exr",
                                                                                });
        if (!importPath.empty())
        {
            importImage(importPath);
        }
    }

    SDKWindow::Get().MainLoop(Render);

    destroyExistingTexture();

    SDKWindow::Get().Destroy();

    return 0;
}
