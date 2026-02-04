//
// Created by droc101 on 1/20/26.
//

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <game_sdk/DesktopInterface.h>
#include <game_sdk/DialogFilters.h>
#include <game_sdk/SDKWindow.h>
#include <game_sdk/SharedMgr.h>
#include <imgui.h>
#include <libassets/asset/DataAsset.h>
#include <libassets/type/Color.h>
#include <libassets/type/Param.h>
#include <libassets/util/Error.h>
#include <misc/cpp/imgui_stdlib.h>
#include <string>
#include <utility>

static DataAsset dataAsset{};
static std::string selectedPath = "/";

static void openGkvl(const std::string &path)
{
    const Error::ErrorCode errorCode = DataAsset::CreateFromAsset(path.c_str(), dataAsset);
    if (errorCode != Error::ErrorCode::OK)
    {
        SDKWindow::ErrorMessage(std::format("Failed to open the KvList!\n{}", errorCode));
    }
}

static void importJson(const std::string &path)
{
    const Error::ErrorCode errorCode = DataAsset::CreateFromJson(path.c_str(), dataAsset);
    if (errorCode != Error::ErrorCode::OK)
    {
        SDKWindow::ErrorMessage(std::format("Failed to import the KvList!\n{}", errorCode));
    }
}

static void saveGkvl(const std::string &path)
{
    const Error::ErrorCode errorCode = dataAsset.SaveAsAsset(path.c_str());
    if (errorCode != Error::ErrorCode::OK)
    {
        SDKWindow::ErrorMessage(std::format("Failed to save the KvList!\n{}", errorCode));
    }
}

static void exportJson(const std::string &path)
{
    const Error::ErrorCode errorCode = dataAsset.SaveAsJson(path.c_str());
    if (errorCode != Error::ErrorCode::OK)
    {
        SDKWindow::ErrorMessage(std::format("Failed to export the KvList!\n{}", errorCode));
    }
}

static void RenderParam(Param &param, const std::string &displayName, const std::string &name, const std::string &path);

static std::string newItemName{};
static Param::ParamType newItemType = Param::ParamType::PARAM_TYPE_INTEGER;

static void RenderArray(ParamVector &vector, const std::string &path)
{
    if (vector.empty())
    {
        ImGui::TextDisabled("No Elements");
    }
    for (size_t i = 0; i < vector.size(); i++)
    {
        Param &p = vector.at(i);
        RenderParam(p, std::format("[{}]: {}", i, p.GetTypeName()), std::format("{}", i), path);
    }
    ImGui::Separator();
    if (ImGui::Button("Add"))
    {
        ImGui::OpenPopup("ArrayNewItem");
    }
    if (ImGui::BeginPopup("ArrayNewItem", ImGuiWindowFlags_NoMove))
    {
        if (ImGui::BeginCombo("##type", Param::paramTypeNames.at(newItemType).c_str()))
        {
            for (const std::pair<const Param::ParamType, std::string> &pair: Param::paramTypeNames)
            {
                if (pair.first == Param::ParamType::PARAM_TYPE_NONE)
                {
                    continue;
                }
                if (ImGui::Selectable(pair.second.c_str(), newItemType == pair.first))
                {
                    newItemType = pair.first;
                }
            }
            ImGui::EndCombo();
        }
        if (ImGui::Button("Add"))
        {
            Param p = Param();
            p.ClearToType(newItemType);
            vector.push_back(p);
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

static void RenderKvList(KvList &list, const std::string &path)
{
    if (list.empty())
    {
        ImGui::TextDisabled("No Elements");
    }
    for (std::pair<const std::string, Param> &pair: list)
    {
        RenderParam(pair.second, std::format("{}: {}", pair.first, pair.second.GetTypeName()), pair.first, path);
    }
    ImGui::Separator();
    if (ImGui::Button("Add"))
    {
        newItemName = "";
        ImGui::OpenPopup("KvListNewItem");
    }
    if (ImGui::BeginPopup("KvListNewItem", ImGuiWindowFlags_NoMove))
    {
        ImGui::Text("New element key:");
        ImGui::InputText("##newName", &newItemName);

        if (ImGui::BeginCombo("##type", Param::paramTypeNames.at(newItemType).c_str()))
        {
            for (const std::pair<const Param::ParamType, std::string> &pair: Param::paramTypeNames)
            {
                if (pair.first == Param::ParamType::PARAM_TYPE_NONE)
                {
                    continue;
                }
                if (ImGui::Selectable(pair.second.c_str(), newItemType == pair.first))
                {
                    newItemType = pair.first;
                }
            }
            ImGui::EndCombo();
        }

        const bool badName = newItemName.empty() ||
                             list.contains(newItemName) ||
                             newItemName.find('/') != std::string::npos;
        ImGui::BeginDisabled(badName);
        if (ImGui::Button("Add"))
        {
            Param p = Param();
            p.ClearToType(newItemType);
            list[newItemName] = p;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndDisabled();
        ImGui::EndPopup();
    }
}

static void RenderParam(Param &param, const std::string &displayName, const std::string &name, const std::string &path)
{
    const bool selected = (path + name) == selectedPath;
    ImGuiTreeNodeFlags tnf = ImGuiTreeNodeFlags_SpanFullWidth |
                             ImGuiTreeNodeFlags_DrawLinesFull |
                             ImGuiTreeNodeFlags_OpenOnDoubleClick;
    if (selected)
    {
        tnf |= ImGuiTreeNodeFlags_Selected;
    }
    if (param.GetType() == Param::ParamType::PARAM_TYPE_ARRAY)
    {
        const bool arrayOpen = ImGui::TreeNodeEx((displayName + "##" + path).c_str(), tnf);
        if (ImGui::IsItemClicked())
        {
            selectedPath = (path + name);
        }
        if (arrayOpen)
        {
            RenderArray(param.GetRef<ParamVector>({}), path + name + "/");
            ImGui::TreePop();
        }
    } else if (param.GetType() == Param::ParamType::PARAM_TYPE_KV_LIST)
    {
        const bool kvlOpen = ImGui::TreeNodeEx((displayName + "##" + path).c_str(), tnf);
        if (ImGui::IsItemClicked())
        {
            selectedPath = (path + name);
        }
        if (kvlOpen)
        {
            RenderKvList(param.GetRef<KvList>({}), path + name + "/");
            ImGui::TreePop();
        }
    } else
    {
        if (ImGui::Selectable((displayName + "##" + path).c_str(),
                              selected,
                              ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowOverlap))
        {
            selectedPath = (path + name);
        }
    }
}

static Param *GetSelection(const std::filesystem::path &path)
{
    Param *param = nullptr;
    for (const std::filesystem::path &component: path)
    {
        if (component == "/")
        {
            continue;
        }
        if (param == nullptr)
        {
            if (dataAsset.data.contains(component.string()))
            {
                param = &dataAsset.data.at(component.string());
            }
        } else
        {
            if (param->GetType() == Param::ParamType::PARAM_TYPE_ARRAY)
            {
#ifdef WIN32
                const std::string componentString = component.string(); // STOP BEING WIDE
                const size_t index = strtoull(componentString.c_str(), nullptr, 10);
#else
                const size_t index = strtoull(component.c_str(), nullptr, 10);
#endif
                param = param->ArrayElementPointer(index);
            } else if (param->GetType() == Param::ParamType::PARAM_TYPE_KV_LIST)
            {
                param = param->KvListElementPointer(component.string());
            } else
            {
                assert(false);
            }
        }
    }
    return param;
}

static void RenderSidebar()
{
    Param *p = GetSelection(selectedPath);
    const std::filesystem::path path = std::filesystem::path(selectedPath);
    Param *parent = GetSelection(path.parent_path());
    if (p == nullptr)
    {
        ImGui::TextDisabled("No Selection");
    } else
    {
        ImGui::Text("%s", selectedPath.c_str());
        ImGui::Separator();
        Param::ParamType type = p->GetType();
        ImGui::PushItemWidth(-1);
        if (ImGui::BeginCombo("##type", p->GetTypeName().c_str()))
        {
            for (const std::pair<const Param::ParamType, std::string> &pair: Param::paramTypeNames)
            {
                if (pair.first == Param::ParamType::PARAM_TYPE_NONE)
                {
                    continue;
                }
                if (ImGui::Selectable(pair.second.c_str(), type == pair.first))
                {
                    p->ClearToType(pair.first);
                    type = pair.first;
                }
            }
            ImGui::EndCombo();
        }

        if (type == Param::ParamType::PARAM_TYPE_BYTE)
        {
            ImGui::InputScalar("##value", ImGuiDataType_U8, p->GetPointer<uint8_t>());
        } else if (type == Param::ParamType::PARAM_TYPE_INTEGER)
        {
            ImGui::InputScalar("##value", ImGuiDataType_S32, p->GetPointer<int32_t>());
        } else if (type == Param::ParamType::PARAM_TYPE_FLOAT)
        {
            ImGui::InputScalar("##value", ImGuiDataType_Float, p->GetPointer<float>());
        } else if (type == Param::ParamType::PARAM_TYPE_BOOL)
        {
            ImGui::Checkbox("Value", p->GetPointer<bool>());
        } else if (type == Param::ParamType::PARAM_TYPE_STRING)
        {
            ImGui::InputText("##value", p->GetPointer<std::string>());
        } else if (type == Param::ParamType::PARAM_TYPE_COLOR)
        {
            ImGui::ColorEdit4("##value", p->GetPointer<Color>()->GetDataPointer());
        }

        ImGui::Separator();

        if (ImGui::Button("Delete"))
        {
            if (parent != nullptr)
            {
                if (parent->GetType() == Param::ParamType::PARAM_TYPE_ARRAY)
                {
                    ParamVector *vec = parent->GetPointer<ParamVector>();
                    std::erase_if(*vec, [p](const Param &child) { return &child == p; });
                } else if (parent->GetType() == Param::ParamType::PARAM_TYPE_KV_LIST)
                {
                    KvList *list = parent->GetPointer<KvList>();
                    list->erase(path.filename().string());
                }
            } else
            {
                dataAsset.data.erase(path.filename().string());
            }
        }
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
    ImGui::Begin("kvledit", nullptr, windowFlags);
    bool newPressed = ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_N);
    bool openPressed = ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_O);
    bool importPressed = ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_O);
    bool savePressed = ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_S);
    bool exportPressed = ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_S);

    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            newPressed |= ImGui::MenuItem("New", "Ctrl+N");
            ImGui::Separator();
            openPressed |= ImGui::MenuItem("Open", "Ctrl+O");
            importPressed |= ImGui::MenuItem("Import", "Ctrl+Shift+O");
            savePressed |= ImGui::MenuItem("Save", "Ctrl+S");
            exportPressed |= ImGui::MenuItem("Export", "Ctrl+Shift+S");
            ImGui::Separator();
            if (ImGui::MenuItem("Quit", "Alt+F4"))
            {
                SDKWindow::PostQuit();
            }
            ImGui::EndMenu();
        }
        SharedMgr::SharedMenuUI("kvledit");
        ImGui::EndMainMenuBar();
    }

    if (newPressed)
    {
        dataAsset = DataAsset();
    } else if (openPressed)
    {
        SDKWindow::OpenFileDialog(openGkvl, DialogFilters::gkvlFilters);
    } else if (importPressed)
    {
        SDKWindow::OpenFileDialog(importJson, DialogFilters::kvlJsonFilters);
    } else if (savePressed)
    {
        SDKWindow::SaveFileDialog(saveGkvl, DialogFilters::gkvlFilters);
    } else if (exportPressed)
    {
        SDKWindow::SaveFileDialog(exportJson, DialogFilters::kvlJsonFilters);
    }

    const ImVec2 &availableSize = ImGui::GetContentRegionAvail();

    constexpr float statsWidth = 250.0f;
    const float treeWidth = availableSize.x - statsWidth - 8.0f;
    ImGui::BeginChild("TreePane",
                      ImVec2(treeWidth, availableSize.y),
                      ImGuiChildFlags_Borders,
                      ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoBringToFrontOnFocus);
    {
        RenderKvList(dataAsset.data, "/");
    }
    ImGui::EndChild();
    ImGui::SameLine();

    ImGui::BeginChild("EditPane", ImVec2(statsWidth, availableSize.y));
    RenderSidebar();
    ImGui::EndChild();

    ImGui::End();
}

int main(int argc, char **argv)
{
    if (!SDKWindow::Init("GAME SDK Key-Value List Editor"))
    {
        return -1;
    }

    const std::string &openPath = DesktopInterface::GetFileArgument(argc, argv, {".gkvl"});
    if (!openPath.empty())
    {
        openGkvl(openPath);
    } else
    {
        const std::string &importPath = DesktopInterface::GetFileArgument(argc, argv, {".json"});
        if (!importPath.empty())
        {
            importJson(importPath);
        }
    }

    SDKWindow::MainLoop(Render);

    SDKWindow::Destroy();

    return 0;
}
