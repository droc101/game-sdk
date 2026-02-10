//
// Created by droc101 on 10/19/25.
//

#include <cfloat>
#include <cstdint>
#include <cstdio>
#include <libassets/type/Color.h>
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
#include <libassets/util/Error.h>
#include <memory>
#include <string>

std::unique_ptr<ParamDefinition> ParamDefinition::Create(const nlohmann::json &json,
                                                         Error::ErrorCode &e,
                                                         const std::string &paramName)
{
    std::unique_ptr<ParamDefinition> output = nullptr;
    const Param::ParamType type = Param::ParseType(json.value("type", "int"));
    if (type == Param::ParamType::PARAM_TYPE_NONE)
    {
        printf("Failed to parse type of param:\n%s", json.dump(4).c_str());
        e = Error::ErrorCode::INCORRECT_FORMAT;
        return nullptr;
    }
    const std::string desc = json.value("description", "");
    const std::string displayName = json.value("display", paramName);
    if (json.contains("options"))
    {
        output = std::make_unique<OptionParamDefinition>(json.value("options", ""), json.value("default", ""));
    } else
    {
        if (type == Param::ParamType::PARAM_TYPE_BYTE)
        {
            output = std::make_unique<ByteParamDefinition>(json.value("minimum", static_cast<uint8_t>(0)),
                                                           json.value("maximum", static_cast<uint8_t>(UINT8_MAX)),
                                                           json.value("default", static_cast<uint8_t>(0)));
        } else if (type == Param::ParamType::PARAM_TYPE_INTEGER)
        {
            output = std::make_unique<IntParamDefinition>(json.value("minimum", 0),
                                                          json.value("maximum", INT32_MAX),
                                                          json.value("default", 0));
        } else if (type == Param::ParamType::PARAM_TYPE_FLOAT)
        {
            output = std::make_unique<FloatParamDefinition>(json.value("minimum", -FLT_MAX),
                                                            json.value("maximum", FLT_MAX),
                                                            json.value("default", 0.0f),
                                                            json.value("step", 0.1f));
        } else if (type == Param::ParamType::PARAM_TYPE_BOOL)
        {
            output = std::make_unique<BoolParamDefinition>(json.value("default", false));
        } else if (type == Param::ParamType::PARAM_TYPE_STRING)
        {
            output = std::make_unique<StringParamDefinition>(json.value("default", ""),
                                                             json.value("string_hint", "none"));
        } else if (type == Param::ParamType::PARAM_TYPE_COLOR)
        {
            output = std::make_unique<ColorParamDefinition>(Color(json.value("default", 0xFFFFFFFF)),
                                                            json.value("showAlpha", true));
        } else if (type == Param::ParamType::PARAM_TYPE_UINT_64)
        {
            output = std::make_unique<Uint64ParamDefinition>(json.value("minimum", 0ull),
                                                             json.value("maximum", 0ull),
                                                             json.value("default", 0ull));
        } else
        {
            e = Error::ErrorCode::INCORRECT_FORMAT;
            return nullptr;
        }
    }

    output->description = desc;
    output->type = type;
    output->displayName = displayName;

    e = Error::ErrorCode::OK;
    return output;
}
