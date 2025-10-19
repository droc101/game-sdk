//
// Created by droc101 on 10/19/25.
//

#include <cfloat>
#include <cstdint>
#include <cstdio>
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

ParamDefinition *ParamDefinition::Create(const nlohmann::json &json, Error::ErrorCode &e)
{
    ParamDefinition *output = nullptr;
    const Param::ParamType type = Param::ParseType(json.value("type", "int"));
    if (type == Param::ParamType::PARAM_TYPE_NONE)
    {
        printf("Failed to parse type of param:\n%s", json.dump(4).c_str());
        e = Error::ErrorCode::INCORRECT_FORMAT;
        return nullptr;
    }
    const std::string desc = json.value("description", "");
    if (json.contains("options"))
    {
        OptionParamDefinition *optionDef = new OptionParamDefinition();
        optionDef->defaultValue = json.value("default", "");
        optionDef->optionListName = json.value("options", "");
        output = optionDef;
    } else
    {
        if (type == Param::ParamType::PARAM_TYPE_BYTE)
        {
            ByteParamDefinition *byteDef = new ByteParamDefinition();
            byteDef->minimumValue = json.value("minimum", static_cast<uint8_t>(0));
            byteDef->maximumValue = json.value("maximum", static_cast<uint8_t>(UINT8_MAX));
            byteDef->defaultValue = json.value("default", static_cast<uint8_t>(0));
            output = byteDef;
        } else if (type == Param::ParamType::PARAM_TYPE_INTEGER)
        {
            IntParamDefinition *intDef = new IntParamDefinition();
            intDef->minimumValue = json.value("minimum", 0);
            intDef->maximumValue = json.value("maximum", INT32_MAX);
            intDef->defaultValue = json.value("default", 0);
            output = intDef;
        } else if (type == Param::ParamType::PARAM_TYPE_FLOAT)
        {
            FloatParamDefinition *floatDef = new FloatParamDefinition();
            floatDef->minimumValue = json.value("minimum", -FLT_MAX);
            floatDef->maximumValue = json.value("maximum", FLT_MAX);
            floatDef->defaultValue = json.value("default", 0.0f);
            floatDef->step = json.value("step", 0.1f);
            output = floatDef;
        } else if (type == Param::ParamType::PARAM_TYPE_BOOL)
        {
            BoolParamDefinition *boolDef = new BoolParamDefinition();
            boolDef->defaultValue = json.value("default", false);
            output = boolDef;
        } else if (type == Param::ParamType::PARAM_TYPE_STRING)
        {
            StringParamDefinition *stringDef = new StringParamDefinition();
            stringDef->defaultValue = json.value("default", "");
            const std::string hint = json.value("hint", "none");
            if (hint == "texture")
            {
                stringDef->hintType = StringParamDefinition::StringParamHint::TEXTURE;
            } else if (hint == "model")
            {
                stringDef->hintType = StringParamDefinition::StringParamHint::MODEL;
            } else if (hint == "sound")
            {
                stringDef->hintType = StringParamDefinition::StringParamHint::SOUND;
            } else if (hint == "actor")
            {
                stringDef->hintType = StringParamDefinition::StringParamHint::ACTOR;
            } else
            {
                stringDef->hintType = StringParamDefinition::StringParamHint::NONE;
            }
            output = stringDef;
        } else
        {
            e = Error::ErrorCode::INCORRECT_FORMAT;
            return nullptr;
        }
        // TODO: color
    }

    output->description = desc;
    output->type = type;

    e = Error::ErrorCode::OK;
    return output;
}
