//
// Created by droc101 on 10/21/25.
//

#include "EditActorWindow.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <format>
#include <imgui.h>
#include <libassets/type/Actor.h>
#include <libassets/type/ActorDefinition.h>
#include <libassets/type/Color.h>
#include <libassets/type/IOConnection.h>
#include <libassets/type/OptionDefinition.h>
#include <libassets/type/Param.h>
#include <libassets/type/paramDefs/BoolParamDefinition.h>
#include <libassets/type/paramDefs/ByteParamDefinition.h>
#include <libassets/type/paramDefs/ColorParamDefinition.h>
#include <libassets/type/paramDefs/FloatParamDefinition.h>
#include <libassets/type/paramDefs/IntParamDefinition.h>
#include <libassets/type/paramDefs/OptionParamDefinition.h>
#include <libassets/type/paramDefs/ParamDefinition.h>
#include <libassets/type/paramDefs/StringParamDefinition.h>
#include <libassets/type/SignalDefinition.h>
#include <libassets/util/Error.h>
#include <misc/cpp/imgui_stdlib.h>
#include <ranges>
#include <unordered_set>
#include <utility>
#include <vector>
#include "MapEditor.h"
#include "SharedMgr.h"

void EditActorWindow::Render(Actor &actor)
{
    if (!visible)
    {
        return;
    }

    ImGui::Begin("Actor Properties", &visible, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking);

    if (SharedMgr::actorDefinitions.empty() || !SharedMgr::actorDefinitions.contains("actor"))
    {
        ImGui::TextDisabled("No actor definitions are loaded. Is the game path set correctly?");
        ImGui::End();
        return;
    }

    ImGui::Text("Class");
    if (ImGui::BeginCombo("##a", actor.className.c_str()))
    {
        for (const std::pair<const std::string, ActorDefinition> &def: SharedMgr::actorDefinitions)
        {
            if (def.second.isVirtual)
            {
                continue;
            }
            if (actor.className == def.first)
            {
                ImGui::SetItemDefaultFocus();
            }
            if (ImGui::Selectable(def.first.c_str(), def.first == actor.className))
            {
                actor.className = def.first;
                actor.ApplyDefinition(def.second, true);
                selectedParam = 0;
            }
        }
        ImGui::EndCombo();
    }

    const ActorDefinition &def = SharedMgr::actorDefinitions.at(actor.className);

    ImGui::Text("%s", def.description.empty() ? "no description" : def.description.c_str());

    ImGui::Dummy(ImVec2(0, 8));

    if (ImGui::BeginTabBar("##classInfo"))
    {
        if (ImGui::BeginTabItem("Params"))
        {
            RenderParamsTab(actor, SharedMgr::actorDefinitions.at(actor.className));
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("I/O Connections"))
        {
            RenderOutputsTab(actor, SharedMgr::actorDefinitions.at(actor.className));
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Position"))
        {
            ImGui::Text("Position");
            ImGui::InputFloat3("##position", actor.position.data());
            ImGui::Text("Rotation");
            ImGui::InputFloat3("##rotation", actor.rotation.data());
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    ImGui::End();
}

void EditActorWindow::RenderParamsTab(Actor &actor, const ActorDefinition &definition)
{
    const float boxSize = ImGui::GetContentRegionAvail().x - 300;
    ImVec2 cursorPos = ImGui::GetCursorPos();
    cursorPos.x += boxSize + 8;
    if (ImGui::BeginTable("charTable", 2, ImGuiTableFlags_ScrollY | ImGuiTableFlags_BordersH, ImVec2(boxSize, -1)))
    {
        ImGui::TableSetupColumn("Key", ImGuiTableColumnFlags_NoSort | ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_NoSort | ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableHeadersRow();
        std::vector<std::string> paramNamesV{};
        paramNamesV.reserve(actor.params.size());
        for (const std::string &param: actor.params | std::views::keys)
        {
            paramNamesV.push_back(param);
        }
        std::ranges::sort(paramNamesV);
        size_t i = 0;
        for (const std::string &key: paramNamesV)
        {
            ParamDefinition *paramDef = nullptr;
            const Error::ErrorCode e = definition.GetParam(key, paramDef);
            if (e != Error::ErrorCode::OK)
            {
                continue;
            }
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            if (ImGui::Selectable(paramDef->displayName.c_str(),
                                  selectedParam == i,
                                  ImGuiSelectableFlags_SpanAllColumns))
            {
                selectedParam = i;
            }

            ImGui::TableNextColumn();
            const Param &param = actor.params.at(key);
            const OptionParamDefinition *optionDef = dynamic_cast<OptionParamDefinition *>(paramDef);
            if (optionDef != nullptr)
            {
                const OptionDefinition *options = optionDef->definition;
                const std::vector<std::string> optionList = options->GetOptions();
                const std::string selected = options->Find(param);
                ImGui::TextUnformatted(selected.c_str());

            } else if (paramDef->type == Param::ParamType::PARAM_TYPE_BYTE)
            {
                ImGui::Text("%d", param.Get<uint8_t>(0));
            } else if (paramDef->type == Param::ParamType::PARAM_TYPE_INTEGER)
            {
                ImGui::Text("%d", param.Get<int32_t>(0));
            } else if (paramDef->type == Param::ParamType::PARAM_TYPE_FLOAT)
            {
                ImGui::Text("%.3f", param.Get<float>(0.0f));
            } else if (paramDef->type == Param::ParamType::PARAM_TYPE_BOOL)
            {
                ImGui::Text("%s", param.Get<bool>(false) ? "true" : "false");
            } else if (paramDef->type == Param::ParamType::PARAM_TYPE_STRING)
            {
                ImGui::TextWrapped("\"%s\"", param.Get<std::string>("").c_str());
            } else if (paramDef->type == Param::ParamType::PARAM_TYPE_COLOR)
            {
                ImGui::TextWrapped("0x%x", param.Get<Color>(Color(-1)).GetUint32());
            }

            i++;
        }
        ImGui::EndTable();

        ImGui::SetCursorPos(cursorPos);
        if (!paramNamesV.empty())
        {
            ParamDefinition *paramDef = nullptr;
            const Error::ErrorCode e = definition.GetParam(paramNamesV.at(selectedParam), paramDef);
            if (e == Error::ErrorCode::OK)
            {
                Param &param = actor.params.at(paramNamesV.at(selectedParam));
                ImGui::SetCursorPosX(cursorPos.x);
                ImGui::TextWrapped("%s",
                                   paramDef->description.empty() ? "No Description" : paramDef->description.c_str());
                ImGui::SetCursorPosX(cursorPos.x);
                ImGui::PushItemWidth(-1);
                const OptionParamDefinition *optionDef = dynamic_cast<OptionParamDefinition *>(paramDef);
                if (optionDef != nullptr)
                {
                    const OptionDefinition *options = optionDef->definition;
                    const std::vector<std::string> optionList = options->GetOptions();
                    const std::string selected = options->Find(param);
                    if (ImGui::BeginCombo("##a", selected.c_str()))
                    {
                        for (const std::string &key: optionList)
                        {
                            if (selected == key)
                            {
                                ImGui::SetItemDefaultFocus();
                            }
                            if (ImGui::Selectable(key.c_str(), key == selected))
                            {
                                param = options->GetValue(key);
                            }
                        }
                        ImGui::EndCombo();
                    }
                } else if (paramDef->type == Param::ParamType::PARAM_TYPE_BYTE)
                {
                    const ByteParamDefinition *byteDef = dynamic_cast<ByteParamDefinition *>(paramDef);
                    uint8_t val = param.Get<uint8_t>(byteDef->defaultValue);
                    if (ImGui::InputScalar("##value", ImGuiDataType_U8, &val))
                    {
                        val = std::clamp(val, byteDef->minimumValue, byteDef->maximumValue);
                        param.Set<uint8_t>(val);
                    }
                } else if (paramDef->type == Param::ParamType::PARAM_TYPE_INTEGER)
                {
                    const IntParamDefinition *intDef = dynamic_cast<IntParamDefinition *>(paramDef);
                    int32_t val = param.Get<int32_t>(intDef->defaultValue);
                    if (ImGui::InputScalar("##value", ImGuiDataType_S32, &val))
                    {
                        val = std::clamp(val, intDef->minimumValue, intDef->maximumValue);
                        param.Set<int32_t>(val);
                    }
                } else if (paramDef->type == Param::ParamType::PARAM_TYPE_FLOAT)
                {
                    const FloatParamDefinition *floatDef = dynamic_cast<FloatParamDefinition *>(paramDef);
                    float val = param.Get<float>(floatDef->defaultValue);
                    if (ImGui::InputFloat("##value", &val, floatDef->step))
                    {
                        val = std::clamp(val, floatDef->minimumValue, floatDef->maximumValue);
                        param.Set<float>(val);
                    }
                } else if (paramDef->type == Param::ParamType::PARAM_TYPE_BOOL)
                {
                    const BoolParamDefinition *boolDef = dynamic_cast<BoolParamDefinition *>(paramDef);
                    bool val = param.Get<bool>(boolDef->defaultValue);
                    if (ImGui::Checkbox("##value", &val))
                    {
                        param.Set<bool>(val);
                    }
                } else if (paramDef->type == Param::ParamType::PARAM_TYPE_STRING)
                {
                    // TODO hints
                    const StringParamDefinition *stringDef = dynamic_cast<StringParamDefinition *>(paramDef);
                    std::string val = param.Get<std::string>(stringDef->defaultValue);
                    if (ImGui::InputText("##value", &val))
                    {
                        param.Set<std::string>(val);
                    }
                } else if (paramDef->type == Param::ParamType::PARAM_TYPE_COLOR)
                {
                    const ColorParamDefinition *colorDef = dynamic_cast<ColorParamDefinition *>(paramDef);
                    Color col = param.Get<Color>(Color(-1));
                    std::array<float, 4> colorData = col.CopyData();
                    if (ImGui::ColorEdit4("##color",
                                          colorData.data(),
                                          colorDef->showAlpha ? 0 : ImGuiColorEditFlags_NoAlpha))
                    {
                        col = Color(colorData[0], colorData[1], colorData[2], colorData[3]);
                        param.Set<Color>(col);
                    }
                }
            }
        } else
        {
            ImGui::TextDisabled("This class has no params");
        }
    }
}

void EditActorWindow::RenderOutputsTab(Actor &actor, const ActorDefinition &definition)
{
    const float boxSize = ImGui::GetContentRegionAvail().y - 200;
    if (ImGui::BeginTable("charTable", 4, ImGuiTableFlags_ScrollY | ImGuiTableFlags_BordersH, ImVec2(-1, boxSize)))
    {
        ImGui::TableSetupColumn("Source Output", ImGuiTableColumnFlags_NoSort | ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Target Name", ImGuiTableColumnFlags_NoSort | ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Target Input", ImGuiTableColumnFlags_NoSort | ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Param Override", ImGuiTableColumnFlags_NoSort | ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableHeadersRow();
        size_t i = 0;
        for (const IOConnection &connection: actor.connections)
        {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::PushID(std::format("##connection_{}", i).c_str());
            if (ImGui::Selectable(connection.sourceOutput.c_str(),
                                  selectedConnection == i,
                                  ImGuiSelectableFlags_SpanAllColumns))
            {
                selectedConnection = i;
            }
            ImGui::PopID();

            ImGui::TableNextColumn();
            ImGui::Text("%s", connection.targetName.c_str());

            ImGui::TableNextColumn();
            ImGui::Text("%s", connection.targetInput.c_str());

            ImGui::TableNextColumn();
            const Param &param = connection.param;
            if (param.GetType() == Param::ParamType::PARAM_TYPE_BYTE)
            {
                ImGui::Text("%d", param.Get<uint8_t>(0));
            } else if (param.GetType() == Param::ParamType::PARAM_TYPE_INTEGER)
            {
                ImGui::Text("%d", param.Get<int32_t>(0));
            } else if (param.GetType() == Param::ParamType::PARAM_TYPE_FLOAT)
            {
                ImGui::Text("%.3f", param.Get<float>(0.0f));
            } else if (param.GetType() == Param::ParamType::PARAM_TYPE_BOOL)
            {
                ImGui::Text("%s", param.Get<bool>(false) ? "true" : "false");
            } else if (param.GetType() == Param::ParamType::PARAM_TYPE_STRING)
            {
                ImGui::TextWrapped("\"%s\"", param.Get<std::string>("").c_str());
            } else if (param.GetType() == Param::ParamType::PARAM_TYPE_STRING)
            {
                ImGui::TextWrapped("0x%x", param.Get<Color>(Color(-1)).GetUint32());
            }

            i++;
        }

        ImGui::EndTable();
        if (ImGui::Button("Add"))
        {
            actor.connections.emplace_back(); // TODO default values
        }
        ImGui::SameLine();
        if (ImGui::Button("Delete"))
        {
            actor.connections.erase(actor.connections.begin() + static_cast<int64_t>(selectedConnection));
            selectedConnection = std::clamp(selectedConnection, static_cast<size_t>(0), actor.connections.size() - 1);
        }
        ImGui::Separator();
        if (!actor.connections.empty())
        {
            IOConnection &connection = actor.connections.at(selectedConnection);
            Param &param = connection.param;
            std::unordered_set<std::string> myOutputNames{};
            definition.GetOutputNames(myOutputNames);
            ImGui::Text("Source Output");
            if (ImGui::BeginCombo("##srcOutput", connection.sourceOutput.c_str()))
            {
                for (const std::string &output: myOutputNames)
                {
                    if (connection.sourceOutput == output)
                    {
                        ImGui::SetItemDefaultFocus();
                    }
                    if (ImGui::Selectable(output.c_str(), connection.sourceOutput == output))
                    {
                        connection.sourceOutput = output;
                        selectedParam = 0;
                    }
                }
                ImGui::EndCombo();
            }
            SignalDefinition outputDef{};
            if (definition.GetOutput(connection.sourceOutput, outputDef) != Error::ErrorCode::OK)
            {
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(1, 0, 0, 1), "Unknown Output");
            }

            ImGui::Text("Target Actor");
            std::vector<std::string> actorNames{};
            for (const Actor &levelActor: MapEditor::map.actors)
            {
                if (!levelActor.params.contains("name"))
                {
                    continue;
                }
                const Param &p = levelActor.params.at("name");
                if (p.GetType() != Param::ParamType::PARAM_TYPE_STRING)
                {
                    continue;
                }
                const std::string actorName = p.Get<std::string>("");
                if (std::ranges::find(actorNames, actorName) != actorNames.end())
                {
                    continue;
                }
                if (actorName.empty())
                {
                    continue;
                }
                actorNames.push_back(p.Get<std::string>(""));
            }
            std::ranges::sort(actorNames);
            if (ImGui::BeginCombo("##targetActor", connection.targetName.c_str()))
            {
                for (const std::string &name: actorNames)
                {
                    if (connection.targetName == name)
                    {
                        ImGui::SetItemDefaultFocus();
                    }
                    if (ImGui::Selectable(name.c_str(), connection.targetName == name))
                    {
                        connection.targetName = name;
                    }
                }
                ImGui::EndCombo();
            }

            bool hasInputDef = false;
            SignalDefinition inputDef;

            if (std::ranges::find(actorNames, connection.targetName) != actorNames.end())
            {
                const Actor *targetActor = MapEditor::map.GetActor(connection.targetName);
                assert(targetActor);
                const ActorDefinition &targetDef = SharedMgr::actorDefinitions.at(targetActor->className);
                std::unordered_set<std::string> targetInputNames{};
                targetDef.GetInputNames(targetInputNames);
                ImGui::Text("Target Input");
                if (ImGui::BeginCombo("##targetInput", connection.targetInput.c_str()))
                {
                    for (const std::string &input: targetInputNames)
                    {
                        if (connection.targetInput == input)
                        {
                            ImGui::SetItemDefaultFocus();
                        }
                        if (ImGui::Selectable(input.c_str(), connection.targetInput == input))
                        {
                            connection.targetInput = input;
                        }
                    }
                    ImGui::EndCombo();
                }

                const Error::ErrorCode e = targetDef.GetInput(connection.targetInput, inputDef);
                if (e == Error::ErrorCode::OK)
                {
                    if (connection.param.GetType() != inputDef.GetType())
                    {
                        connection.param.ClearToType(inputDef.GetType());
                    }
                    hasInputDef = true;
                }
            } else
            {
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(1, 0, 0, 1), "Unknown Target Actor");

                ImGui::Text("Target Input");
                ImGui::BeginDisabled();
                if (ImGui::BeginCombo("##targetInput", connection.targetInput.c_str()))
                {
                    ImGui::EndCombo();
                }
                ImGui::EndDisabled();
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(1, 0, 0, 1), "Unknown Target Input");
            }

            if (hasInputDef)
            {
                if (outputDef.GetType() != inputDef.GetType())
                {
                    connection.overridesParam = true;
                    ImGui::BeginDisabled();
                    ImGui::Checkbox("Parameter Override", &connection.overridesParam);
                    ImGui::EndDisabled();
                } else
                {
                    ImGui::Checkbox("Parameter Override", &connection.overridesParam);
                }

                if (connection.overridesParam)
                {
                    // ImGui::TextWrapped("%s", paramDef->description.empty() ? "No Description" : paramDef->description.c_str());
                    if (param.GetType() == Param::ParamType::PARAM_TYPE_BYTE)
                    {
                        uint8_t val = param.Get<uint8_t>(0);
                        if (ImGui::InputScalar("##value", ImGuiDataType_U8, &val))
                        {
                            param.Set<uint8_t>(val);
                        }
                    } else if (param.GetType() == Param::ParamType::PARAM_TYPE_INTEGER)
                    {
                        int32_t val = param.Get<int32_t>(0);
                        if (ImGui::InputScalar("##value", ImGuiDataType_S32, &val))
                        {
                            param.Set<int32_t>(val);
                        }
                    } else if (param.GetType() == Param::ParamType::PARAM_TYPE_FLOAT)
                    {
                        float val = param.Get<float>(0);
                        if (ImGui::InputFloat("##value", &val, 0.01))
                        {
                            param.Set<float>(val);
                        }
                    } else if (param.GetType() == Param::ParamType::PARAM_TYPE_BOOL)
                    {
                        bool val = param.Get<bool>(false);
                        if (ImGui::Checkbox("##value", &val))
                        {
                            param.Set<bool>(val);
                        }
                    } else if (param.GetType() == Param::ParamType::PARAM_TYPE_STRING)
                    {
                        std::string val = param.Get<std::string>("");
                        if (ImGui::InputText("##value", &val))
                        {
                            param.Set<std::string>(val);
                        }
                    } else if (param.GetType() == Param::ParamType::PARAM_TYPE_COLOR)
                    {
                        Color col = param.Get<Color>(Color(-1));
                        std::array<float, 4> colorData = col.CopyData();
                        if (ImGui::ColorEdit4("##color", colorData.data()))
                        {
                            col = Color(colorData[0], colorData[1], colorData[2], colorData[3]);
                            param.Set<Color>(col);
                        }
                    } else
                    {
                        ImGui::TextDisabled("This input does not accept a parameter.");
                    }
                }
            } else
            {
                ImGui::BeginDisabled();
                bool dummy = false;
                ImGui::Checkbox("Parameter Override", &dummy);
                ImGui::EndDisabled();
            }
        } else
        {
            ImGui::TextDisabled("This actor has no connections");
        }
    }
}
