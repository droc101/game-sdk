//
// Created by droc101 on 9/5/25.
//

#pragma once
#include <cstdint>
#include <string>
#include "Color.h"


class Param
{
    public:
        enum class ParamType: uint8_t
        {
            PARAM_TYPE_BYTE,
            PARAM_TYPE_INTEGER,
            PARAM_TYPE_FLOAT,
            PARAM_TYPE_BOOL,
            PARAM_TYPE_STRING,
            PARAM_TYPE_NONE,
            PARAM_TYPE_COLOR
        };

        Param() = default;

    private:
        ParamType type = ParamType::PARAM_TYPE_NONE;

        uint8_t byteValue = 0;
        int32_t intValue = 0;
        float floatValue = 0;
        bool boolValue = false;
        std::string stringValue = "";
        Color colorValue = Color(0, 0, 0, 1);
};
