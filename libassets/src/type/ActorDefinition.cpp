//
// Created by droc101 on 10/18/25.
//

#include <fstream>
#include <libassets/type/ActorDefinition.h>
#include <libassets/type/Param.h>
#include <libassets/type/paramDefs/ParamDefinition.h>
#include <libassets/type/renderDefs/RenderDefinition.h>
#include <libassets/type/SignalDefinition.h>
#include <libassets/util/Error.h>
#include <nlohmann/json.hpp>
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
    const nlohmann::json definition_json = nlohmann::json::parse(j);
    if (definition_json.is_discarded())
    {
        file.close();
        // printf("File %s is not valid JSON\n", path.c_str());
        return Error::ErrorCode::INCORRECT_FORMAT;
    }

    definition = ActorDefinition();
    definition.className = std::filesystem::path(path).stem().string();
    definition.parentClassName = definition_json.value("extends", "");
    definition.description = definition_json.value("description", "");
    definition.isVirtual = definition_json.value("virtual", false);

    if (definition_json.contains("display"))
    {
        const nlohmann::json &renderDef = definition_json.at("display");
        Error::ErrorCode e = Error::ErrorCode::OK;
        definition.renderDefinition = RenderDefinition::Create(renderDef, e);
        if (e != Error::ErrorCode::OK)
        {
            return e;
        }
    } else
    {
        definition.renderDefinition.color = Color(0x00ff00ff);
    }

    if (definition_json.contains("inputs"))
    {
        nlohmann::json inputs = definition_json.at("inputs");
        for (const auto &[key, value]: inputs.items())
        {
            const SignalDefinition signal = SignalDefinition(value.value("description", ""),
                                                             Param::ParseType(value.value("type", "none")));
            definition.inputs[key] = signal;
        }
    }

    if (definition_json.contains("outputs"))
    {
        nlohmann::json outputs = definition_json.at("outputs");
        for (const auto &[key, value]: outputs.items())
        {
            const SignalDefinition signal = SignalDefinition(value.value("description", ""),
                                                             Param::ParseType(value.value("type", "none")));
            definition.outputs[key] = signal;
        }
    }

    if (definition_json.contains("params"))
    {
        nlohmann::json params = definition_json.at("params");
        for (const auto &[key, value]: params.items())
        {
            Error::ErrorCode e = Error::ErrorCode::UNKNOWN;
            ParamDefinition *param = ParamDefinition::Create(value, e, key);
            if (e != Error::ErrorCode::OK)
            {
                // TODO delete any existing params
                return e;
            }
            if (param == nullptr)
            {
                return Error::ErrorCode::UNKNOWN;
            }
            definition.params[key] = param;
        }
    }

    file.close();
    return Error::ErrorCode::OK;
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
    for (const std::pair<std::string, ParamDefinition *> kv: params)
    {
        out.insert(kv.first);
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

Error::ErrorCode ActorDefinition::GetParam(const std::string &name, ParamDefinition *&param) const
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
