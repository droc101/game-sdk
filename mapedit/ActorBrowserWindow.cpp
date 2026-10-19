//
// Created by droc101 on 10/19/25.
//

#include "ActorBrowserWindow.h"
#include <cassert>
#include <cstddef>
#include <game_sdk/Window.h>
#include <game_sdk/WindowManager.h>
#include <imgui.h>
#include <libassets/type/ActorDefinition.h>
#include <libassets/type/InputDefinition.h>
#include <libassets/type/OutputDefinition.h>
#include <libassets/type/Param.h>
#include <libassets/type/paramDefs/BoolParamDefinition.h>
#include <libassets/type/paramDefs/ByteParamDefinition.h>
#include <libassets/type/paramDefs/ColorParamDefinition.h>
#include <libassets/type/paramDefs/FloatParamDefinition.h>
#include <libassets/type/paramDefs/IntParamDefinition.h>
#include <libassets/type/paramDefs/OptionParamDefinition.h>
#include <libassets/type/paramDefs/ParamDefinition.h>
#include <libassets/type/paramDefs/StringParamDefinition.h>
#include <libassets/type/paramDefs/Uint64ParamDefinition.h>
#include <libassets/type/paramDefs/Vec2ParamDefinition.h>
#include <libassets/type/paramDefs/Vec3ParamDefinition.h>
#include <libassets/util/Error.h>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
#include "MapEditor.h"

ActorBrowserWindow::ActorBrowserWindow(const std::string &classname)
{
    if (MapEditor::adm.HasActorClass(classname) && (history.empty() || history.back() != classname))
    {
        history.push_back(classname);
    }
}

const Window::WindowProperties &ActorBrowserWindow::GetProperties() const
{
    return properties;
}

void ActorBrowserWindow::Render()
{
    if (MapEditor::adm.GetActorClassCount() == 0 || !MapEditor::adm.HasActorClass("actor"))
    {
        ImGui::TextDisabled("No actor definitions are loaded. Is the game path set correctly?");
        ImGui::End();
        return;
    }

    ImGui::BeginDisabled(history.empty());
    if (ImGui::Button("Back"))
    {
        history.pop_back();
        resetScrollAfterNav = true;
    }
    ImGui::EndDisabled();
    ImGui::SameLine();
    ImGui::BeginDisabled(history.empty() || history.back() == HOME_PAGE);
    if (ImGui::Button("Home"))
    {
        history.clear();
        resetScrollAfterNav = true;
    }
    ImGui::EndDisabled();
    ImGui::SameLine();
    ImGui::Text("%s", history.empty() ? HOME_PAGE.c_str() : history.back().c_str());

    if (resetScrollAfterNav)
    {
        ImGui::SetNextWindowScroll({0, 0});
        resetScrollAfterNav = false;
    }

    if (ImGui::BeginChild("##browser", ImGui::GetContentRegionAvail(), ImGuiChildFlags_Borders))
    {
        if (history.empty() || history.back() == HOME_PAGE)
        {
            RenderHomePage();
        } else
        {
            RenderActorPage(history.back());
        }
        ImGui::EndChild();
    }
}

void ActorBrowserWindow::RenderClassTree(const std::string &baseClass)
{
    std::vector<std::string> derived{};
    MapEditor::adm.GetDerivedActorClasses(baseClass, derived);
    for (const std::string &actorClass: derived)
    {
        if (ImGui::TextLink(actorClass.c_str()))
        {
            history.push_back(actorClass);
            resetScrollAfterNav = true;
        }
        ImGui::Indent();
        RenderClassTree(actorClass);
        ImGui::Unindent();
    }
}

void ActorBrowserWindow::RenderHomePage()
{
    ImGui::PushFont(WindowManager::Get().GetCurrentWindow()->GetNormalFont(), 32);
    ImGui::Text("%s", HOME_PAGE.c_str());
    ImGui::PopFont();
    ImGui::Separator();

    RenderClassTree("");
}

void ActorBrowserWindow::RenderParamDef(const std::shared_ptr<ParamDefinition> &param)
{
    ImGui::Text("%s", param->description.empty() ? "No Description" : param->description.c_str());
    const OptionParamDefinition *opt = dynamic_cast<OptionParamDefinition *>(param.get());
    if (opt != nullptr)
    {
        ImGui::Text("Options List: %s", opt->optionListName.c_str());
        for (const std::string &option: opt->definition->GetOptions())
        {
            ImGui::BulletText("%s", option.c_str());
        }
        ImGui::Text("Default Value: %s", opt->defaultValue.c_str());
    } else
    {
        if (param->type == Param::ParamType::PARAM_TYPE_BYTE)
        {
            const ByteParamDefinition *p = dynamic_cast<ByteParamDefinition *>(param.get());
            ImGui::Text("Range: %d to %d", p->minimumValue, p->maximumValue);
            ImGui::Text("Default: %d", p->defaultValue);
        } else if (param->type == Param::ParamType::PARAM_TYPE_INTEGER)
        {
            const IntParamDefinition *p = dynamic_cast<IntParamDefinition *>(param.get());
            ImGui::Text("Range: %d to %d", p->minimumValue, p->maximumValue);
            ImGui::Text("Default: %d", p->defaultValue);
        } else if (param->type == Param::ParamType::PARAM_TYPE_FLOAT)
        {
            const FloatParamDefinition *p = dynamic_cast<FloatParamDefinition *>(param.get());
            ImGui::Text("Range: %.3f to %.3f", p->minimumValue, p->maximumValue);
            ImGui::TextWrapped("Default: %.3f", p->defaultValue);
            ImGui::TextWrapped("Step: %.3f", p->step);
        } else if (param->type == Param::ParamType::PARAM_TYPE_BOOL)
        {
            const BoolParamDefinition *p = dynamic_cast<BoolParamDefinition *>(param.get());
            ImGui::Text("Default: %s", p->defaultValue ? "true" : "false");
        } else if (param->type == Param::ParamType::PARAM_TYPE_STRING)
        {
            const StringParamDefinition *p = dynamic_cast<StringParamDefinition *>(param.get());
            ImGui::TextWrapped("Default: \"%s\"", p->defaultValue.c_str());
            switch (p->hintType)
            {
                case StringParamDefinition::StringParamHint::NONE:
                    break;
                case StringParamDefinition::StringParamHint::ACTOR:
                    ImGui::Text("Hint: Actor Name");
                    break;
                case StringParamDefinition::StringParamHint::MODEL:
                    ImGui::Text("Hint: Model");
                    break;
                case StringParamDefinition::StringParamHint::SOUND:
                    ImGui::Text("Hint: Sound");
                    break;
                case StringParamDefinition::StringParamHint::TEXTURE:
                    ImGui::Text("Hint: Texture");
                    break;
                case StringParamDefinition::StringParamHint::MATERIAL:
                    ImGui::Text("Hint: Material");
                    break;
            }
        } else if (param->type == Param::ParamType::PARAM_TYPE_COLOR)
        {
            const ColorParamDefinition *p = dynamic_cast<ColorParamDefinition *>(param.get());
            ImGui::Text("Show Alpha: %s", p->showAlpha ? "Yes" : "No");
            ImGui::Text("Default: #%X", p->defaultValue.GetUint32());
        } else if (param->type == Param::ParamType::PARAM_TYPE_UINT_64)
        {
            const Uint64ParamDefinition *p = dynamic_cast<Uint64ParamDefinition *>(param.get());
            ImGui::Text("Range: %zu to %zu", p->minimumValue, p->maximumValue);
            ImGui::Text("Default: %zu", p->defaultValue);
        } else if (param->type == Param::ParamType::PARAM_TYPE_VEC2)
        {
            const Vec2ParamDefinition *p = dynamic_cast<Vec2ParamDefinition *>(param.get());
            ImGui::Text("Default: (%.3f, %.3f)", p->defaultValue.x, p->defaultValue.y);
        } else if (param->type == Param::ParamType::PARAM_TYPE_VEC3)
        {
            const Vec3ParamDefinition *p = dynamic_cast<Vec3ParamDefinition *>(param.get());
            ImGui::Text("Default: (%.3f, %.3f, %.3f)", p->defaultValue.x, p->defaultValue.y, p->defaultValue.z);
        }
    }
}

void ActorBrowserWindow::RenderActorPage(const std::string &actorClass)
{
    const ActorDefinition &def = MapEditor::adm.GetActorDefinition(actorClass);
    ImGui::PushFont(WindowManager::Get().GetCurrentWindow()->GetNormalFont(), 32);
    if (def.isVirtual)
    {
        ImGui::TextDisabled("virtual");
        ImGui::SameLine();
    }
    if (def.editorOnly)
    {
        ImGui::TextDisabled("editor");
        ImGui::SameLine();
    }
    ImGui::TextDisabled("class");
    ImGui::SameLine();
    ImGui::Text("%s", actorClass.c_str());
    if (!def.parentClassName.empty())
    {
        ImGui::SameLine();
        ImGui::TextDisabled("extends");
        ImGui::SameLine();
        if (ImGui::TextLink(def.parentClassName.c_str()))
        {
            history.push_back(def.parentClassName);
            resetScrollAfterNav = true;
        }
    }
    ImGui::PopFont();
    ImGui::Separator();
    ImGui::TextWrapped("%s", def.description.empty() ? "No Description" : def.description.c_str());

    if (!def.params.empty())
    {
        ImGui::Dummy({1, 4});
        ImGui::PushFont(WindowManager::Get().GetCurrentWindow()->GetNormalFont(), 24);
        ImGui::Text("Params");
        ImGui::PopFont();
        ImGui::Separator();
        for (const std::pair<const std::string, std::shared_ptr<ParamDefinition>> &paramPair: def.params)
        {
            ImGui::PushFont(WindowManager::Get().GetCurrentWindow()->GetMonospaceFont());
            ImGui::Text("%s", paramPair.first.c_str());
            ImGui::SameLine();
            ImGui::TextDisabled(": %s", Param::GetTypeName(paramPair.second->type).c_str());
            ImGui::PopFont();
            ImGui::Indent();

            std::shared_ptr<ParamDefinition> param;
            assert(def.GetParam(paramPair.first, param) == Error::ErrorCode::OK);
            RenderParamDef(param);

            ImGui::Unindent();
        }
    }

    if (!def.inputs.empty())
    {
        ImGui::Dummy({1, 4});
        ImGui::PushFont(WindowManager::Get().GetCurrentWindow()->GetNormalFont(), 24);
        ImGui::Text("Inputs");
        ImGui::PopFont();
        ImGui::Separator();
        for (const std::pair<const std::string, InputDefinition> &input: def.inputs)
        {
            ImGui::PushFont(WindowManager::Get().GetCurrentWindow()->GetMonospaceFont());
            ImGui::Text("%s", input.first.c_str());
            std::vector<std::string> types{};
            for (const Param::ParamType &type: input.second.GetTypes())
            {
                types.push_back(Param::GetTypeName(type));
            }
            std::stringstream typesString{};
            for (size_t i = 0; i < types.size(); i++)
            {
                if (i != 0)
                {
                    typesString << " | ";
                }
                typesString << types.at(i);
            }

            ImGui::SameLine();
            ImGui::TextDisabled(": %s", typesString.str().c_str());
            ImGui::PopFont();
            ImGui::Indent();

            ImGui::Text("%s",
                        input.second.GetDescription().empty() ? "No Description"
                                                              : input.second.GetDescription().c_str());

            ImGui::Unindent();
        }
    }

    if (!def.outputs.empty())
    {
        ImGui::Dummy({1, 4});
        ImGui::PushFont(WindowManager::Get().GetCurrentWindow()->GetNormalFont(), 24);
        ImGui::Text("Outputs");
        ImGui::PopFont();
        ImGui::Separator();
        for (const std::pair<const std::string, OutputDefinition> &output: def.outputs)
        {
            ImGui::PushFont(WindowManager::Get().GetCurrentWindow()->GetMonospaceFont());
            ImGui::Text("%s", output.first.c_str());
            ImGui::SameLine();
            ImGui::TextDisabled(": %s", Param::GetTypeName(output.second.GetType()).c_str());
            ImGui::PopFont();
            ImGui::Indent();

            ImGui::Text("%s",
                        output.second.GetDescription().empty() ? "No Description"
                                                               : output.second.GetDescription().c_str());

            ImGui::Unindent();
        }
    }

    std::vector<std::string> derived{};
    MapEditor::adm.GetDerivedActorClasses(actorClass, derived);
    if (!derived.empty())
    {
        ImGui::Dummy({1, 4});
        ImGui::PushFont(WindowManager::Get().GetCurrentWindow()->GetNormalFont(), 24);
        ImGui::Text("Derived Classes");
        ImGui::PopFont();
        ImGui::Separator();
        for (const std::string & classname : derived)
        {
            ImGui::Bullet();
            if (ImGui::TextLink(classname.c_str()))
            {
                history.push_back(classname);
                resetScrollAfterNav = true;
            }
        }
    }
}
