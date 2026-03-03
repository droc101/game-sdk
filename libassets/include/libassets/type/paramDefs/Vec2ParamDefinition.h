//
// Created by droc101 on 3/3/26.
//

#ifndef GAME_SDK_VEC2PARAMDEFINITION_H
#define GAME_SDK_VEC2PARAMDEFINITION_H

#include <glm/vec2.hpp>
#include <libassets/type/paramDefs/ParamDefinition.h>

class Vec2ParamDefinition final: public ParamDefinition
{
    public:
        Vec2ParamDefinition() = default;
        explicit Vec2ParamDefinition(const glm::vec2 defaultValue): defaultValue(defaultValue) {}

        glm::vec2 defaultValue{};
};

#endif //GAME_SDK_VEC2PARAMDEFINITION_H
