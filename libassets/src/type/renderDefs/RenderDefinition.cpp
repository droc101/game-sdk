//
// Created by droc101 on 1/9/26.
//

#include <libassets/type/Actor.h>
#include <libassets/type/Color.h>
#include <libassets/type/renderDefs/RenderDefinition.h>
#include <string>

RenderDefinition::RenderDefinition(const nlohmann::json &json)
{
    model = RDV_Str(json, "model", "");
    texture = RDV_Str(json, "texture", "");
    color = RDV_Color(json, "color", Color(0, 1, 0, 1));
    affectLightmap = RDV_Bool(json, "affect_lightmap", false);
    directional = RDV_Bool(json, "directional", true);
    hasBoxRenderer = RDV_Bool(json, "box", false);
    boxWidth = RDV_Float(json, "box_width", 1.0f);
    boxHeight = RDV_Float(json, "box_height", 1.0f);
    boxDepth = RDV_Float(json, "box_depth", 1.0f);
}

std::string RenderDefinition::GetModel(const Actor &actor) const
{
    if (model.usesParam)
    {
        if (actor.params.contains(model.paramName))
        {
            return actor.params.at(model.paramName).Get<std::string>("model/error.gmdl");
        }
    } else
    {
        return model.value;
    }
    return "";
}

std::string RenderDefinition::GetTexture(const Actor &actor) const
{
    if (texture.usesParam)
    {
        if (actor.params.contains(texture.paramName))
        {
            return actor.params.at(texture.paramName).Get<std::string>("");
        }
    } else
    {
        return texture.value;
    }
    return "";
}

Color RenderDefinition::GetColor(const Actor &actor) const
{
    if (color.usesParam)
    {
        if (actor.params.contains(color.paramName))
        {
            return actor.params.at(color.paramName).Get<Color>(Color(-1));
        }
    } else
    {
        return color.value;
    }
    return Color(-1);
}

bool RenderDefinition::GetAffectLightmap(const Actor &actor) const
{
    if (affectLightmap.usesParam)
    {
        if (actor.params.contains(affectLightmap.paramName))
        {
            return actor.params.at(affectLightmap.paramName).Get<bool>(false);
        }
    } else
    {
        return affectLightmap.value;
    }
    return false;
}

bool RenderDefinition::GetDirectional(const Actor &actor) const
{
    if (directional.usesParam)
    {
        if (actor.params.contains(directional.paramName))
        {
            return actor.params.at(directional.paramName).Get<bool>(true);
        }
    } else
    {
        return directional.value;
    }
    return true;
}

bool RenderDefinition::HasBoxRenderer(const Actor &actor) const
{
    if (hasBoxRenderer.usesParam)
    {
        if (actor.params.contains(hasBoxRenderer.paramName))
        {
            return actor.params.at(hasBoxRenderer.paramName).Get<bool>(true);
        }
    } else
    {
        return hasBoxRenderer.value;
    }
    return true;
}

glm::vec3 RenderDefinition::GetBoxExtents(const Actor &actor) const
{
    glm::vec3 extents = {1.0f, 1.0f, 1.0f};

    if (boxWidth.usesParam)
    {
        if (actor.params.contains(boxWidth.paramName))
        {
            extents.x = actor.params.at(boxWidth.paramName).Get<float>(1.0f);
        }
    } else
    {
        extents.x = boxWidth.value;
    }

    if (boxHeight.usesParam)
    {
        if (actor.params.contains(boxHeight.paramName))
        {
            extents.y = actor.params.at(boxHeight.paramName).Get<float>(1.0f);
        }
    } else
    {
        extents.y = boxHeight.value;
    }

    if (boxDepth.usesParam)
    {
        if (actor.params.contains(boxDepth.paramName))
        {
            extents.z = actor.params.at(boxDepth.paramName).Get<float>(1.0f);
        }
    } else
    {
        extents.z = boxDepth.value;
    }

    return extents;
}

RenderDefinition::RenderDefinitionValue<std::string> RenderDefinition::RDV_Str(const nlohmann::json &json,
                                                                               const std::string &key,
                                                                               const std::string &defaultValue)
{
    RenderDefinitionValue<std::string> rdv{};
    if (json.contains(key) && json.at(key).type() == nlohmann::detail::value_t::string)
    {
        const std::string value = json.value(key, defaultValue);
        if (value.starts_with("$"))
        {
            rdv.usesParam = true;
            rdv.paramName = value.substr(1, value.length() - 1);
        } else
        {
            rdv.value = value;
        }
    } else
    {
        rdv.value = defaultValue;
    }
    return rdv;
}

RenderDefinition::RenderDefinitionValue<Color> RenderDefinition::RDV_Color(const nlohmann::json &json,
                                                                           const std::string &key,
                                                                           const Color defaultValue)
{
    RenderDefinitionValue<Color> rdv{};
    if (json.contains(key))
    {
        if (json.at(key).type() == nlohmann::detail::value_t::number_unsigned ||
            json.at(key).type() == nlohmann::detail::value_t::number_integer)
        {
            rdv.value = Color(json.value(key, -1u));
        } else if (json.at(key).type() == nlohmann::detail::value_t::string)
        {
            const std::string colorValue = json.value(key, "");
            if (colorValue.starts_with("$"))
            {
                rdv.usesParam = true;
                rdv.paramName = colorValue.substr(1, colorValue.length() - 1);
            } else
            {
                rdv.value = defaultValue;
            }
        }
    } else
    {
        rdv.value = defaultValue;
    }
    return rdv;
}

RenderDefinition::RenderDefinitionValue<bool> RenderDefinition::RDV_Bool(const nlohmann::json &json,
                                                                         const std::string &key,
                                                                         bool defaultValue)
{
    RenderDefinitionValue<bool> rdv{};
    if (json.contains(key))
    {
        if (json.at(key).type() == nlohmann::detail::value_t::boolean)
        {
            rdv.value = json.value(key, defaultValue);
        } else if (json.at(key).type() == nlohmann::detail::value_t::string)
        {
            const std::string boolValue = json.value(key, "");
            if (boolValue.starts_with("$"))
            {
                rdv.usesParam = true;
                rdv.paramName = boolValue.substr(1, boolValue.length() - 1);
            } else
            {
                rdv.value = defaultValue;
            }
        }
    } else
    {
        rdv.value = defaultValue;
    }
    return rdv;
}

RenderDefinition::RenderDefinitionValue<float> RenderDefinition::RDV_Float(const nlohmann::json &json,
                                                                           const std::string &key,
                                                                           float defaultValue)
{
    RenderDefinitionValue<float> rdv{};
    if (json.contains(key))
    {
        if (json.at(key).type() == nlohmann::detail::value_t::number_float ||
            json.at(key).type() == nlohmann::detail::value_t::number_unsigned ||
            json.at(key).type() == nlohmann::detail::value_t::number_integer)
        {
            rdv.value = json.value(key, defaultValue);
        } else if (json.at(key).type() == nlohmann::detail::value_t::string)
        {
            const std::string floatValue = json.value(key, "");
            if (floatValue.starts_with("$"))
            {
                rdv.usesParam = true;
                rdv.paramName = floatValue.substr(1, floatValue.length() - 1);
            } else
            {
                rdv.value = defaultValue;
            }
        }
    } else
    {
        rdv.value = defaultValue;
    }
    return rdv;
}
