//
// Created by droc101 on 9/20/26.
//

#include "Light.h"

Light::Light(const Actor& actor)
{
    position = actor.position;
    rotation = actor.rotation;
    negativeForwardDirection = glm::normalize(glm::vec3{
            sin(glm::radians(actor.rotation.y)) * cos(glm::radians(actor.rotation.x)),
            -sin(glm::radians(actor.rotation.x)),
            cos(glm::radians(actor.rotation.y)) * cos(glm::radians(actor.rotation.x)),
    });
    const Color c = Param::KvListGet(actor.params, "color", Color(-1));
    color = glm::vec3{c.R(), c.G(), c.B()};
    brightness = Param::KvListGet(actor.params, "brightness", 1.0f);

    if (actor.className == "light_directional")
    {
        type = Type::DIRECTIONAL;
    } else
    {
        constantAttenuation = Param::KvListGet(actor.params, "constant_attenuation", 0.0f);
        linearAttenuation = Param::KvListGet(actor.params, "linear_attenuation", 0.0f);
        quadraticAttenuation = Param::KvListGet(actor.params, "quadratic_attenuation", 1.0f);
        attenuationMultiplier = Param::KvListGet(actor.params, "attenuation_multiplier", 2.0f);
        if (actor.className == "light_point")
        {
            type = Type::POINT;
        } else if (actor.className == "light_spot")
        {
            type = Type::SPOT;
            brightAngle = Param::KvListGet(actor.params, "bright_angle", 30.0f);
            fadingAngle = Param::KvListGet(actor.params, "fading_angle", 45.0f);
            cookie = Param::KvListGet(actor.params, "cookie", std::string(""));
        }
    }
}

void Light::Write(DataWriter& writer) const
{
    KvList list{};
    list["type"] = Param(static_cast<uint8_t>(type));
    list["position"] = Param(position);
    list["rotation"] = Param(rotation);
    list["color"] = Param(Color(color.r, color.g, color.b, 1.0f));
    list["brightness"] = Param(brightness);

    if (type != Type::DIRECTIONAL)
    {
        list["constant_attenuation"] = Param(constantAttenuation);
        list["linear_attenuation"] = Param(linearAttenuation);
        list["quadratic_attenuation"] = Param(quadraticAttenuation);
        list["attenuation_multiplier"] = Param(attenuationMultiplier);
        if (type == Type::SPOT)
        {
            list["bright_angle"] = Param(brightAngle);
            list["fading_angle"] = Param(fadingAngle);
            list["cookie"] = Param(cookie);
        }
    }
    Param::WriteKvList(writer, list);
}
