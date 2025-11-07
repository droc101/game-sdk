//
// Created by droc101 on 9/5/25.
//

#include <cassert>
#include <cstdint>
#include <libassets/type/Actor.h>
#include <libassets/type/ActorDefinition.h>
#include <libassets/type/OptionDefinition.h>
#include <libassets/type/Param.h>
#include <libassets/type/paramDefs/BoolParamDefinition.h>
#include <libassets/type/paramDefs/ByteParamDefinition.h>
#include <libassets/type/paramDefs/FloatParamDefinition.h>
#include <libassets/type/paramDefs/IntParamDefinition.h>
#include <libassets/type/paramDefs/OptionParamDefinition.h>
#include <libassets/type/paramDefs/ParamDefinition.h>
#include <libassets/type/paramDefs/StringParamDefinition.h>
#include <libassets/util/Error.h>
#include <string>
#include <unordered_set>
#include <utility>

void Actor::ApplyDefinition(const ActorDefinition &definition)
{
    std::unordered_set<std::string> paramNames{};
    definition.GetParamNames(paramNames);

    params.clear();

    for (const std::string &key: paramNames)
    {
        ParamDefinition *param = nullptr;
        const Error::ErrorCode e = definition.GetParam(key, param);
        // TODO check e

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
        } else
        {
            assert(false); // unimplemented param type
        }
    }
}
