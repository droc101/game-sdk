//
// Created by droc101 on 3/3/26.
//

#ifndef GAME_SDK_VEC3PARAMDEFINITION_H
#define GAME_SDK_VEC3PARAMDEFINITION_H

#include <glm/vec3.hpp>
#include <libassets/type/paramDefs/ParamDefinition.h>

class Vec3ParamDefinition final: public ParamDefinition
{
    public:
        Vec3ParamDefinition() = default;
        explicit Vec3ParamDefinition(const glm::vec3 defaultValue): defaultValue(defaultValue) {}

        glm::vec3 defaultValue{};
};

#endif //GAME_SDK_VEC3PARAMDEFINITION_H
