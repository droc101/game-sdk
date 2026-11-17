//
// Created by NBT22 on 11/16/26.
//

#include "ShaderLight.hpp"
#include <cmath>
#include "Light.h"
#include "VulkanContext.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/rotate_vector.hpp>

ShaderLight::ShaderLight(VulkanContext &context, const Light &light):
    type(light.type),
    position(light.position),
    negativeForwardDirection(light.negativeForwardDirection),
    color(light.color),
    brightness(light.brightness),
    constantAttenuation(light.constantAttenuation),
    linearAttenuation(light.linearAttenuation),
    quadraticAttenuation(light.quadraticAttenuation),
    attenuationMultiplier(light.attenuationMultiplier),
    brightAngle(light.brightAngle),
    fadingAngle(light.fadingAngle)
{
    if (!light.cookie.empty())
    {
        if (context.GetTextureIndex(light.cookie, cookieTextureIndex))
        {
            cookieTextureIndex++;
        } else
        {
            cookieTextureIndex = 0;
        }

        const glm::vec3 unRotatedAxisV = glm::normalize(glm::vec3{
            std::sin(glm::radians(light.rotation.y)) * std::sin(glm::radians(light.rotation.x)),
            std::cos(glm::radians(light.rotation.x)),
            std::cos(glm::radians(light.rotation.y)) * std::sin(glm::radians(light.rotation.x)),
        });
        cookieAxisV = glm::rotate(unRotatedAxisV, -glm::radians(light.rotation.z), light.negativeForwardDirection);
    }
}
