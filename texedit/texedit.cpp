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

static TextureAsset texture{};
static bool textureLoaded = false;
static GLuint glTexture;

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

    std::vector<uint32_t> pixels;
    texture.GetPixelsRGBA(pixels);

    glGenTextures(1, &glTexture);
    glBindTexture(GL_TEXTURE_2D, glTexture);
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_RGBA,
                 static_cast<GLsizei>(texture.GetWidth()),
                 static_cast<GLsizei>(texture.GetHeight()),
                 0,
                 GL_RGBA,
                 GL_UNSIGNED_BYTE,
                 pixels.data());
    const GLint magfilter = texture.filter ? GL_LINEAR : GL_NEAREST;
    GLint minFilter = magfilter;
    const GLint repeat = texture.repeat ? GL_REPEAT : GL_CLAMP_TO_EDGE;
    if (texture.mipmaps)
    {
        glGenerateMipmap(GL_TEXTURE_2D);
        minFilter = texture.filter ? GL_LINEAR_MIPMAP_LINEAR : GL_NEAREST_MIPMAP_LINEAR;
    }
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magfilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, repeat);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, repeat);
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
    const Error::ErrorCode errorCode = TextureAsset::CreateFromImage(path.c_str(), texture);
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
    const Error::ErrorCode errorCode = texture.SaveAsImage(path.c_str(), TextureAsset::ImageFormat::IMAGE_FORMAT_PNG);
    if (errorCode != Error::ErrorCode::OK)
    {
        SDKWindow::Get().ErrorMessage(std::format("Failed to export the texture!\n{}", errorCode));
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

    bool zoomInPressed = ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_Equal) && textureLoaded;
    bool zoomOutPressed = ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_Minus) && textureLoaded;
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
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Tools"))
        {
            if (ImGui::MenuItem("Swap Byte Order"))
            {
                texture.SwapByteOrder();
                loadTexture();
            }
            ImGui::Separator();
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
        SDKWindow::Get().SaveFileDialog(exportPng, DialogFilters::pngFilters);
    } else if (zoomInPressed)
    {
        zoom += 0.1;
        if (zoom > 5.0)
        {
            zoom = 5.0;
        }
    } else if (zoomOutPressed)
    {
        zoom -= 0.1;
        if (zoom < 0.1)
        {
            zoom = 0.1;
        }
    } else if (resetZoomPressed)
    {
        zoom = 1.0f;
    }

    if (textureLoaded)
    {
        const ImVec2 &availableSize = ImGui::GetContentRegionAvail();

        constexpr float statsWidth = 150.0f;
        const float imageWidth = availableSize.x - statsWidth - 8.0f;

        ImGui::BeginChild("ImagePane",
                          ImVec2(imageWidth, availableSize.y),
                          ImGuiChildFlags_Borders,
                          ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoBringToFrontOnFocus);
        {
            const ImVec2 imageSize{static_cast<float>(texture.GetWidth()) * zoom,
                                   static_cast<float>(texture.GetHeight()) * zoom};
            ImGui::Image(glTexture, imageSize);
        }
        ImGui::EndChild();
        ImGui::SameLine();

        ImGui::BeginChild("StatsPane", ImVec2(statsWidth, availableSize.y));
        {
            ImGui::TextUnformatted(std::format("Width: {}\nHeight: {}\nMemory: {} B",
                                               texture.GetWidth(),
                                               texture.GetHeight(),
                                               texture.GetWidth() * texture.GetHeight() * sizeof(uint32_t))
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
                                                                                    ".jpg",
                                                                                    ".jpeg",
                                                                                    ".tga",
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
