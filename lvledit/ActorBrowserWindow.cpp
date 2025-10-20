//
// Created by droc101 on 10/19/25.
//

#include "ActorBrowserWindow.h"
#include <cstddef>
#include <imgui.h>
#include <libassets/type/ActorDefinition.h>
#include <libassets/type/Param.h>
#include <libassets/type/paramDefs/BoolParamDefinition.h>
#include <libassets/type/paramDefs/ByteParamDefinition.h>
#include <libassets/type/paramDefs/FloatParamDefinition.h>
#include <libassets/type/paramDefs/IntParamDefinition.h>
#include <libassets/type/paramDefs/OptionParamDefinition.h>
#include <libassets/type/paramDefs/ParamDefinition.h>
#include <libassets/type/paramDefs/StringParamDefinition.h>
#include <libassets/type/SignalDefinition.h>
#include <libassets/util/Error.h>
#include <ranges>
#include <unordered_set>
#include <utility>
#include <vector>
#include "SharedMgr.h"

void ActorBrowserWindow::Render()
{
    if (!visible)
    {
        return;
    }

    ImGui::Begin("Actor Class Browser", &visible, ImGuiWindowFlags_NoCollapse);

    if (SharedMgr::actorDefinitions.size() == 0 || !SharedMgr::actorDefinitions.contains("actor"))
    {
        ImGui::TextDisabled("No actor definbitions are loaded. Is the gamw path set correctly?");
        ImGui::End();
        return;
    }

    ImGui::Text("Class");
    if (ImGui::BeginCombo("##a", selectedClass.c_str()))
    {
        for (const std::string &key: SharedMgr::actorDefinitions | std::views::keys)
        {
            if (selectedClass == key)
            {
                ImGui::SetItemDefaultFocus();
            }
            if (ImGui::Selectable(key.c_str(), key == selectedClass))
            {
                selectedClass = key;
                selectedParam = 0;
            }
        }
        ImGui::EndCombo();
    }

    const ActorDefinition &def = SharedMgr::actorDefinitions.at(selectedClass);

    ImGui::Text("%s %s: %s",
                def.isVirtual ? "Virtual class" : "Class",
                selectedClass.c_str(),
                def.description.empty() ? "no description" : def.description.c_str());

    ImGui::Dummy(ImVec2(0, 8));

    if (ImGui::BeginTabBar("##classInfo"))
    {
        if (ImGui::BeginTabItem("Params"))
        {
            RenderParamsTab(def);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Inputs"))
        {
            RenderInputsTab(def);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Outputs"))
        {
            RenderOutputsTab(def);
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    ImGui::End();
}

void ActorBrowserWindow::RenderParamsTab(const ActorDefinition &def)
{
    const float boxSize = ImGui::GetContentRegionAvail().x - 300;
    ImVec2 cursorPos = ImGui::GetCursorPos();
    cursorPos.x += boxSize + 8;
    if (ImGui::BeginTable("charTable", 3, ImGuiTableFlags_ScrollY | ImGuiTableFlags_BordersH, ImVec2(boxSize, -1)))
    {
        ImGui::TableSetupColumn("Key", ImGuiTableColumnFlags_NoSort | ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Display Name", ImGuiTableColumnFlags_NoSort | ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_NoSort | ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableHeadersRow();
        std::unordered_set<std::string> paramNames{};
        def.GetParamNames(paramNames);
        std::vector<std::string> paramNamesV(paramNames.begin(), paramNames.end());
        std::ranges::sort(paramNamesV);
        size_t i = 0;
        for (const std::string &key: paramNamesV)
        {
            ParamDefinition *param = nullptr;
            const Error::ErrorCode e = def.GetParam(key, param);
            if (e != Error::ErrorCode::OK)
            {
                continue;
            }
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            if (ImGui::Selectable(key.c_str(), selectedParam == i, ImGuiSelectableFlags_SpanAllColumns))
            {
                selectedParam = i;
            }

            ImGui::TableNextColumn();
            ImGui::Text("%s", param->displayName.c_str());

            ImGui::TableNextColumn();
            switch (param->type)
            {
                case Param::ParamType::PARAM_TYPE_BYTE:
                    ImGui::Text("Byte");
                    break;
                case Param::ParamType::PARAM_TYPE_INTEGER:
                    ImGui::Text("Integer");
                    break;
                case Param::ParamType::PARAM_TYPE_FLOAT:
                    ImGui::Text("Float");
                    break;
                case Param::ParamType::PARAM_TYPE_BOOL:
                    ImGui::Text("Boolean");
                    break;
                case Param::ParamType::PARAM_TYPE_STRING:
                    ImGui::Text("String");
                    break;
                case Param::ParamType::PARAM_TYPE_COLOR:
                    ImGui::Text("Color");
                    break;
                default:
                    ImGui::Text("Unknown");
            }

            i++;
        }
        ImGui::EndTable();

        ImGui::SetCursorPos(cursorPos);
        if (!paramNamesV.empty())
        {
            ParamDefinition *param = nullptr;
            const Error::ErrorCode e = def.GetParam(paramNamesV.at(selectedParam), param);
            if (e == Error::ErrorCode::OK)
            {
                ImGui::SetCursorPosX(cursorPos.x);
                ImGui::TextWrapped("%s", param->description.empty() ? "No Description" : param->description.c_str());
                ImGui::SetCursorPosX(cursorPos.x);
                const OptionParamDefinition *opt = dynamic_cast<OptionParamDefinition *>(param);
                if (opt != nullptr)
                {
                    ImGui::Text("Options List: %s", opt->optionListName.c_str());
                    ImGui::SetCursorPosX(cursorPos.x);
                    if (ImGui::BeginListBox("##optionsList", ImVec2(300 - 16, 0)))
                    {
                        for (auto &i: opt->definition->GetOptions())
                        {
                            ImGui::Text("%s", i.c_str());
                        }
                        ImGui::EndListBox();
                    }
                    ImGui::SetCursorPosX(cursorPos.x);
                    ImGui::Text("Default Value: %s", opt->defaultValue.c_str());
                } else
                {
                    if (param->type == Param::ParamType::PARAM_TYPE_BYTE)
                    {
                        const ByteParamDefinition *p = dynamic_cast<ByteParamDefinition *>(param);
                        ImGui::Text("Minimum: %d", p->minimumValue);
                        ImGui::SetCursorPosX(cursorPos.x);
                        ImGui::Text("Maximum: %d", p->maximumValue);
                        ImGui::SetCursorPosX(cursorPos.x);
                        ImGui::Text("Default: %d", p->defaultValue);
                    } else if (param->type == Param::ParamType::PARAM_TYPE_INTEGER)
                    {
                        const IntParamDefinition *p = dynamic_cast<IntParamDefinition *>(param);
                        ImGui::Text("Minimum: %d", p->minimumValue);
                        ImGui::SetCursorPosX(cursorPos.x);
                        ImGui::Text("Maximum: %d", p->maximumValue);
                        ImGui::SetCursorPosX(cursorPos.x);
                        ImGui::Text("Default: %d", p->defaultValue);
                    } else if (param->type == Param::ParamType::PARAM_TYPE_FLOAT)
                    {
                        const FloatParamDefinition *p = dynamic_cast<FloatParamDefinition *>(param);
                        ImGui::TextWrapped("Minimum: %.3f", p->minimumValue);
                        ImGui::SetCursorPosX(cursorPos.x);
                        ImGui::TextWrapped("Maximum: %.3f", p->maximumValue);
                        ImGui::SetCursorPosX(cursorPos.x);
                        ImGui::TextWrapped("Default: %.3f", p->defaultValue);
                        ImGui::SetCursorPosX(cursorPos.x);
                        ImGui::TextWrapped("Step: %.3f", p->step);
                    } else if (param->type == Param::ParamType::PARAM_TYPE_BOOL)
                    {
                        const BoolParamDefinition *p = dynamic_cast<BoolParamDefinition *>(param);
                        ImGui::Text("Default: %s", p->defaultValue ? "true" : "false");
                    } else if (param->type == Param::ParamType::PARAM_TYPE_STRING)
                    {
                        const StringParamDefinition *p = dynamic_cast<StringParamDefinition *>(param);
                        ImGui::TextWrapped("Default: \"%s\"", p->defaultValue.c_str());
                        ImGui::SetCursorPosX(cursorPos.x);
                        switch (p->hintType)
                        {
                            case StringParamDefinition::StringParamHint::NONE:
                                ImGui::Text("Hint: None");
                                break;
                            case StringParamDefinition::StringParamHint::ACTOR:
                                ImGui::Text("Hint: Actor Name");
                                break;
                            case StringParamDefinition::StringParamHint::MODEL:
                                ImGui::Text("Hint: Model");
                                break;
                            case StringParamDefinition::StringParamHint::SOUND:
                                ImGui::Text("Hint: Sound");
                            case StringParamDefinition::StringParamHint::TEXTURE:
                                ImGui::Text("Hint: Texture");
                                break;
                        }
                    }
                }
            }
        } else
        {
            ImGui::TextDisabled("This class has no params");
        }
    }
}

void ActorBrowserWindow::RenderInputsTab(const ActorDefinition &def)
{
    if (ImGui::BeginTable("inputTable", 3, ImGuiTableFlags_ScrollY | ImGuiTableFlags_BordersH, ImVec2(-1, -1)))
    {
        ImGui::TableSetupColumn("Key", ImGuiTableColumnFlags_NoSort | ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_NoSort | ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Description", ImGuiTableColumnFlags_NoSort | ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableHeadersRow();
        std::unordered_set<std::string> inputNames{};
        def.GetInputNames(inputNames);
        for (const std::string &key: inputNames)
        {
            SignalDefinition signal{};
            const Error::ErrorCode e = def.GetInput(key, signal);
            if (e != Error::ErrorCode::OK)
            {
                continue;
            }
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%s", key.c_str());

            ImGui::TableNextColumn();
            switch (signal.GetType())
            {
                case Param::ParamType::PARAM_TYPE_BYTE:
                    ImGui::Text("Byte");
                    break;
                case Param::ParamType::PARAM_TYPE_INTEGER:
                    ImGui::Text("Integer");
                    break;
                case Param::ParamType::PARAM_TYPE_FLOAT:
                    ImGui::Text("Float");
                    break;
                case Param::ParamType::PARAM_TYPE_BOOL:
                    ImGui::Text("Boolean");
                    break;
                case Param::ParamType::PARAM_TYPE_STRING:
                    ImGui::Text("String");
                    break;
                case Param::ParamType::PARAM_TYPE_COLOR:
                    ImGui::Text("Color");
                    break;
                default:
                    ImGui::Text("Unknown");
            }

            ImGui::TableNextColumn();
            ImGui::TextWrapped("%s",
                               signal.GetDescription().empty() ? "No Description" : signal.GetDescription().c_str());
        }
        ImGui::EndTable();
    }
}

void ActorBrowserWindow::RenderOutputsTab(const ActorDefinition &def)
{
    if (ImGui::BeginTable("outputTable", 3, ImGuiTableFlags_ScrollY | ImGuiTableFlags_BordersH, ImVec2(-1, -1)))
    {
        ImGui::TableSetupColumn("Key", ImGuiTableColumnFlags_NoSort | ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_NoSort | ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Description", ImGuiTableColumnFlags_NoSort | ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableHeadersRow();
        std::unordered_set<std::string> outputNames{};
        def.GetOutputNames(outputNames);
        for (const std::string &key: outputNames)
        {
            SignalDefinition signal{};
            const Error::ErrorCode e = def.GetOutput(key, signal);
            if (e != Error::ErrorCode::OK)
            {
                continue;
            }
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%s", key.c_str());

            ImGui::TableNextColumn();
            switch (signal.GetType())
            {
                case Param::ParamType::PARAM_TYPE_BYTE:
                    ImGui::Text("Byte");
                    break;
                case Param::ParamType::PARAM_TYPE_INTEGER:
                    ImGui::Text("Integer");
                    break;
                case Param::ParamType::PARAM_TYPE_FLOAT:
                    ImGui::Text("Float");
                    break;
                case Param::ParamType::PARAM_TYPE_BOOL:
                    ImGui::Text("Boolean");
                    break;
                case Param::ParamType::PARAM_TYPE_STRING:
                    ImGui::Text("String");
                    break;
                case Param::ParamType::PARAM_TYPE_COLOR:
                    ImGui::Text("Color");
                    break;
                default:
                    ImGui::Text("Unknown");
            }

            ImGui::TableNextColumn();
            ImGui::TextWrapped("%s",
                               signal.GetDescription().empty() ? "No Description" : signal.GetDescription().c_str());
        }
        ImGui::EndTable();
    }
}
