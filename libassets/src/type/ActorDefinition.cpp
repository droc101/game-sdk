//
// Created by droc101 on 10/18/25.
//

#include <fstream>
#include <libassets/type/ActorDefinition.h>
#include <libassets/type/Param.h>
#include <libassets/type/paramDefs/ParamDefinition.h>
#include <libassets/type/renderDefs/OrientationRenderDefinition.h>
#include <libassets/type/renderDefs/PointRenderDefinition.h>
#include <libassets/type/renderDefs/RenderDefinition.h>
#include <libassets/type/SignalDefinition.h>
#include <libassets/util/Error.h>
#include <memory>
#include <nlohmann/json.hpp>
#include <ranges>
#include <sstream>
#include <string>
#include <unordered_set>
#include <utility>

Error::ErrorCode ActorDefinition::Create(const std::string &path, ActorDefinition &definition)
{
    std::ifstream file(path);
    if (!file.is_open())
    {
        return Error::ErrorCode::CANT_OPEN_FILE;
    }
    std::ostringstream ss;
    ss << file.rdbuf();
    const std::string j = ss.str();
    const nlohmann::json definitionJson = nlohmann::json::parse(j);
    if (definitionJson.is_discarded())
    {
        file.close();
        // printf("File %s is not valid JSON\n", path.c_str());
        return Error::ErrorCode::INCORRECT_FORMAT;
    }

    definition = ActorDefinition();
    definition.className = std::filesystem::path(path).stem().string();
    definition.parentClassName = definitionJson.value("extends", "");
    definition.description = definitionJson.value("description", "");
    definition.isVirtual = definitionJson.value("virtual", false);

    if (definitionJson.contains("renderers"))
    {
        const nlohmann::json &renderDefs = definitionJson.at("renderers");
        for (const auto &[key, value]: renderDefs.items())
        {
            Error::ErrorCode e = Error::ErrorCode::UNKNOWN;
            std::unique_ptr<RenderDefinition> rdef = RenderDefinition::Create(value, e);
            if (e == Error::ErrorCode::OK)
            {
                definition.renderDefinitions.push_back(std::move(rdef));
            }
        }
    }

    if (definition.renderDefinitions.empty())
    {
        definition.renderDefinitions.push_back(std::make_shared<PointRenderDefinition>(PointRenderDefinition()));
        definition.renderDefinitions
                .push_back(std::make_shared<OrientationRenderDefinition>(OrientationRenderDefinition()));
    }

    if (definitionJson.contains("inputs"))
    {
        nlohmann::json inputs = definitionJson.at("inputs");
        for (const auto &[key, value]: inputs.items())
        {
            const SignalDefinition signal = SignalDefinition(value.value("description", ""),
                                                             Param::ParseType(value.value("type", "none")));
            definition.inputs[key] = signal;
        }
    }

    if (definitionJson.contains("outputs"))
    {
        nlohmann::json outputs = definitionJson.at("outputs");
        for (const auto &[key, value]: outputs.items())
        {
            const SignalDefinition signal = SignalDefinition(value.value("description", ""),
                                                             Param::ParseType(value.value("type", "none")));
            definition.outputs[key] = signal;
        }
    }

    if (definitionJson.contains("params"))
    {
        nlohmann::json params = definitionJson.at("params");
        for (const auto &[key, value]: params.items())
        {
            Error::ErrorCode e = Error::ErrorCode::UNKNOWN;
            std::unique_ptr<ParamDefinition> param = ParamDefinition::Create(value, e, key);
            if (e != Error::ErrorCode::OK)
            {
                // TODO delete any existing params
                return e;
            }
            if (param == nullptr)
            {
                return Error::ErrorCode::UNKNOWN;
            }
            definition.params[key] = std::move(param);
        }
    }

    file.close();
    return Error::ErrorCode::OK;
}

bool ActorDefinition::Extends(const std::string &baseClass) const
{
    if (className == baseClass)
    {
        return true;
    }
    if (parentClass != nullptr)
    {
        return parentClass->Extends(baseClass);
    }
    return false;
}

void ActorDefinition::GetInputNames(std::unordered_set<std::string> &out) const
{
    if (parentClass != nullptr)
    {
        parentClass->GetInputNames(out);
    }
    for (const std::pair<std::string, SignalDefinition> kv: inputs)
    {
        out.insert(kv.first);
    }
}

void ActorDefinition::GetOutputNames(std::unordered_set<std::string> &out) const
{
    if (parentClass != nullptr)
    {
        parentClass->GetOutputNames(out);
    }
    for (const std::pair<std::string, SignalDefinition> kv: outputs)
    {
        out.insert(kv.first);
    }
}

void ActorDefinition::GetParamNames(std::unordered_set<std::string> &out) const
{
    if (parentClass != nullptr)
    {
        parentClass->GetParamNames(out);
    }
    for (const std::string &key: params | std::views::keys)
    {
        out.insert(key);
    }
}

Error::ErrorCode ActorDefinition::GetInput(const std::string &name, SignalDefinition &input) const
{
    if (inputs.contains(name))
    {
        input = inputs.at(name);
        return Error::ErrorCode::OK;
    }
    if (parentClass != nullptr)
    {
        return parentClass->GetInput(name, input);
    }
    return Error::ErrorCode::NOT_FOUND;
}

Error::ErrorCode ActorDefinition::GetOutput(const std::string &name, SignalDefinition &output) const
{
    if (outputs.contains(name))
    {
        output = outputs.at(name);
        return Error::ErrorCode::OK;
    }
    if (parentClass != nullptr)
    {
        return parentClass->GetOutput(name, output);
    }
    return Error::ErrorCode::NOT_FOUND;
}

Error::ErrorCode ActorDefinition::GetParam(const std::string &name, std::shared_ptr<ParamDefinition> &param) const
{
    if (params.contains(name))
    {
        param = params.at(name);
        return Error::ErrorCode::OK;
    }
    if (parentClass != nullptr)
    {
        return parentClass->GetParam(name, param);
    }
    return Error::ErrorCode::NOT_FOUND;
}
