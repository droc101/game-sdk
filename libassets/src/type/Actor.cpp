//
// Created by droc101 on 9/5/25.
//

#include <cassert>
#include <cstdint>
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
#include <libassets/util/Error.h>
#include <string>
#include <unordered_set>
#include <utility>

Actor::Actor(nlohmann::ordered_json j)
{
    className = j.value("class", "actor");
    position = {
        j["position"]["x"],
        j["position"]["y"],
        j["position"]["z"],
    };
    rotation = {
        j["rotation"]["x"],
        j["rotation"]["y"],
        j["rotation"]["z"],
    };
    const nlohmann::ordered_json conns = j.at("connections");
    for (const nlohmann::basic_json<nlohmann::ordered_map> &conn: conns)
    {
        connections.emplace_back(conn);
    }
    nlohmann::ordered_json jParams = j.at("params");
    for (const auto &[key, value]: jParams.items())
    {
        params[key] = Param(value);
    }
}


void Actor::ApplyDefinition(const ActorDefinition &definition)
{
    std::unordered_set<std::string> paramNames{};
    definition.GetParamNames(paramNames);

    params.clear();

    for (const std::string &key: paramNames)
    {
        ParamDefinition *param = nullptr;
        const Error::ErrorCode e = definition.GetParam(key, param);
        if (e != Error::ErrorCode::OK)
        {
            printf("Failed to get definition for param %s::%s\n", definition.className.c_str(), key.c_str());
            continue;
        }

        const OptionParamDefinition *optionParam = dynamic_cast<OptionParamDefinition *>(param);
        if (optionParam != nullptr)
        {
            const OptionDefinition *def = optionParam->definition;
            params[key] = def->GetValue(optionParam->defaultValue);
        } else if (param->type == Param::ParamType::PARAM_TYPE_BYTE)
        {
            const ByteParamDefinition *byteParam = dynamic_cast<ByteParamDefinition *>(param);
            Param p{};
            p.Set<uint8_t>(byteParam->defaultValue);
            params[key] = p;
        } else if (param->type == Param::ParamType::PARAM_TYPE_INTEGER)
        {
            const IntParamDefinition *intParam = dynamic_cast<IntParamDefinition *>(param);
            Param p{};
            p.Set<int32_t>(intParam->defaultValue);
            params[key] = p;
        } else if (param->type == Param::ParamType::PARAM_TYPE_FLOAT)
        {
            const FloatParamDefinition *floatParam = dynamic_cast<FloatParamDefinition *>(param);
            Param p{};
            p.Set<float>(floatParam->defaultValue);
            params[key] = p;
        } else if (param->type == Param::ParamType::PARAM_TYPE_BOOL)
        {
            const BoolParamDefinition *boolParam = dynamic_cast<BoolParamDefinition *>(param);
            Param p{};
            p.Set<bool>(boolParam->defaultValue);
            params[key] = p;
        } else if (param->type == Param::ParamType::PARAM_TYPE_STRING)
        {
            const StringParamDefinition *stringParam = dynamic_cast<StringParamDefinition *>(param);
            Param p{};
            p.Set<std::string>(stringParam->defaultValue);
            params[key] = p;
        } else if (param->type == Param::ParamType::PARAM_TYPE_COLOR)
        {
            const ColorParamDefinition *colorParam = dynamic_cast<ColorParamDefinition *>(param);
            Param p{};
            p.Set<Color>(colorParam->defaultValue);
            params[key] = p;
        } else
        {
            assert(false); // unimplemented param type
        }
    }
}

nlohmann::ordered_json Actor::GenerateJson() const
{
    nlohmann::ordered_json j{};
    j["class"] = className;
    j["position"]["x"] = position.at(0);
    j["position"]["y"] = position.at(1);
    j["position"]["z"] = position.at(2);
    j["rotation"]["x"] = rotation.at(0);
    j["rotation"]["y"] = rotation.at(1);
    j["rotation"]["z"] = rotation.at(2);
    j["connections"] = nlohmann::ordered_json::array();
    for (const IOConnection &connection: connections)
    {
        j["connections"].push_back(connection.GenerateJson());
    }
    j["params"] = nlohmann::ordered_json::object();
    for (const std::pair<const std::string, Param> &pair: params)
    {
        j["params"][pair.first] = pair.second.GetJson();
    }
    return j;
}

void Actor::Write(DataWriter &writer) const
{
    writer.WriteString(className);
    writer.WriteBuffer<float>(position);
    for (const float x: rotation)
    {
        writer.Write<float>(x * (M_PI / 180.0f));
    }
    writer.Write<size_t>(connections.size());
    for (const IOConnection &connection: connections)
    {
        connection.Write(writer);
    }
    writer.Write<size_t>(params.size());
    for (const std::pair<const std::string, Param> &kv: params)
    {
        writer.WriteString(kv.first);
        kv.second.Write(writer);
    }
}
