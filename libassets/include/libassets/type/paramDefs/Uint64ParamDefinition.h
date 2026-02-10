//
// Created by droc101 on 2/6/26.
//

#ifndef GAME_SDK_UINT64PARAMDEFINITION_H
#define GAME_SDK_UINT64PARAMDEFINITION_H

#include <cstdint>
#include <libassets/type/paramDefs/ParamDefinition.h>

class Uint64ParamDefinition final: public ParamDefinition
{
    public:
        Uint64ParamDefinition() = default;
        Uint64ParamDefinition(const uint64_t minimumValue, const uint64_t maximumValue, const uint64_t defaultValue):
            minimumValue(minimumValue),
            maximumValue(maximumValue),
            defaultValue(defaultValue)
        {}

        uint64_t minimumValue = 0;
        uint64_t maximumValue = UINT64_MAX;
        uint64_t defaultValue = 0;
};


#endif //GAME_SDK_UINT64PARAMDEFINITION_H
