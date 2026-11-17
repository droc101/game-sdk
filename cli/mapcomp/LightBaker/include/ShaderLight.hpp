//
// Created by NBT22 on 11/16/26.
//

#pragma once

#include <cstdint>
#include "Light.h"
#include "VulkanContext.hpp"

class ShaderLight
{
    public:
        ShaderLight(VulkanContext &context, const Light &light);

    private:
        Light::Type type;
        glm::vec3 position;
        glm::vec3 cookieAxisV{};
        glm::vec3 negativeForwardDirection;
        glm::vec3 color;
        float brightness;
        float constantAttenuation;
        float linearAttenuation;
        float quadraticAttenuation;
        float attenuationMultiplier;
        float brightAngle;
        float fadingAngle;
        uint32_t cookieTextureIndex{};
};
