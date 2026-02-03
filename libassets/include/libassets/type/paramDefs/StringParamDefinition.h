//
// Created by droc101 on 10/18/25.
//

#pragma once

#include <cstdint>
#include <libassets/type/paramDefs/ParamDefinition.h>
#include <string>
#include <utility>

class StringParamDefinition final: public ParamDefinition
{
    public:
        enum class StringParamHint : uint8_t
        {
            NONE,
            TEXTURE,
            MODEL,
            SOUND,
            ACTOR,
            MATERIAL
        };

        StringParamDefinition() = default;
        StringParamDefinition(std::string &&defaultValue, const std::string &hint):
            defaultValue(std::move(defaultValue))
        {
            if (hint == "texture")
            {
                hintType = StringParamHint::TEXTURE;
            } else if (hint == "model")
            {
                hintType = StringParamHint::MODEL;
            } else if (hint == "sound")
            {
                hintType = StringParamHint::SOUND;
            } else if (hint == "actor")
            {
                hintType = StringParamHint::ACTOR;
            } else if (hint == "material")
            {
                hintType = StringParamHint::MATERIAL;
            } else
            {
                hintType = StringParamHint::NONE;
            }
        }

        StringParamHint hintType = StringParamHint::NONE;
        std::string defaultValue;
};
