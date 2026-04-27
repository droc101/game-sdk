//
// Created by droc101 on 1/9/26.
//

#include <libassets/type/Actor.h>
#include <libassets/type/Color.h>
#include <libassets/type/renderDefs/RenderDefinition.h>
#include <string>

RenderDefinition::RenderDefinition(const nlohmann::json &json)
{
    if (json.contains("model") && json.at("model").type() == nlohmann::detail::value_t::string)
    {
        const std::string modelValue = json.value("model", "");
        if (modelValue.starts_with("$"))
        {
            model.usesParam = true;
            model.paramName = modelValue.substr(1, modelValue.length() - 1);
        } else
        {
            model.value = modelValue;
        }
    } else
    {
        model.value = "";
    }


    if (json.contains("texture") && json.at("texture").type() == nlohmann::detail::value_t::string)
    {
        const std::string textureValue = json.value("texture", "");
        if (textureValue.starts_with("$"))
        {
            texture.usesParam = true;
            texture.paramName = textureValue.substr(1, textureValue.length() - 1);
        } else
        {
            texture.value = textureValue;
        }
    } else
    {
        texture.value = "";
    }

    if (json.contains("color"))
    {
        if (json.at("color").type() == nlohmann::detail::value_t::number_unsigned ||
            json.at("color").type() == nlohmann::detail::value_t::number_integer)
        {
            color.value = Color(json.value("color", -1u));
        } else if (json.at("color").type() == nlohmann::detail::value_t::string)
        {
            const std::string colorValue = json.value("color", "");
            if (colorValue.starts_with("$"))
            {
                color.usesParam = true;
                color.paramName = colorValue.substr(1, colorValue.length() - 1);
            }
        }
    } else
    {
        color.value = Color(0.0, 1.0, 0.0, 1.0);
    }

    if (json.contains("affect_lightmap"))
    {
        if (json.at("affect_lightmap").type() == nlohmann::detail::value_t::boolean)
        {
            affectLightmap.value = json.value("affect_lightmap", true);
        } else if (json.at("affect_lightmap").type() == nlohmann::detail::value_t::string)
        {
            const std::string affectLightmapValue = json.value("affect_lightmap", "");
            if (affectLightmapValue.starts_with("$"))
            {
                affectLightmap.usesParam = true;
                affectLightmap.paramName = affectLightmapValue.substr(1, affectLightmapValue.length() - 1);
            }
        }
    } else
    {
        affectLightmap.value = false;
    }

    if (json.contains("directional"))
    {
        if (json.at("directional").type() == nlohmann::detail::value_t::boolean)
        {
            directional.value = json.value("directional", true);
        } else if (json.at("directional").type() == nlohmann::detail::value_t::string)
        {
            const std::string directionalValue = json.value("directional", "");
            if (directionalValue.starts_with("$"))
            {
                directional.usesParam = true;
                directional.paramName = directionalValue.substr(1, directionalValue.length() - 1);
            }
        }
    } else
    {
        directional.value = true;
    }

    if (json.contains("box"))
    {
        if (json.at("box").type() == nlohmann::detail::value_t::boolean)
        {
            hasBoxRenderer.value = json.value("box", true);
        } else if (json.at("box").type() == nlohmann::detail::value_t::string)
        {
            const std::string hasBoxRendererValue = json.value("box", "");
            if (hasBoxRendererValue.starts_with("$"))
            {
                hasBoxRenderer.usesParam = true;
                hasBoxRenderer.paramName = hasBoxRendererValue.substr(1, hasBoxRendererValue.length() - 1);
            }
        }
    } else
    {
        hasBoxRenderer.value = false;
    }

    if (json.contains("box_width"))
    {
        if (json.at("box_width").type() == nlohmann::detail::value_t::number_float)
        {
            boxWidth.value = json.value("box_width", 1.0f);
        } else if (json.at("box_width").type() == nlohmann::detail::value_t::string)
        {
            const std::string boxWidthValue = json.value("box_width", "");
            if (boxWidthValue.starts_with("$"))
            {
                boxWidth.usesParam = true;
                boxWidth.paramName = boxWidthValue.substr(1, boxWidthValue.length() - 1);
            }
        }
    } else
    {
        boxWidth.value = 1.0f;
    }

    if (json.contains("box_height"))
    {
        if (json.at("box_height").type() == nlohmann::detail::value_t::number_float)
        {
            boxHeight.value = json.value("box_height", 1.0f);
        } else if (json.at("box_height").type() == nlohmann::detail::value_t::string)
        {
            const std::string boxWidthValue = json.value("box_height", "");
            if (boxWidthValue.starts_with("$"))
            {
                boxHeight.usesParam = true;
                boxHeight.paramName = boxWidthValue.substr(1, boxWidthValue.length() - 1);
            }
        }
    } else
    {
        boxHeight.value = 1.0f;
    }

    if (json.contains("box_depth"))
    {
        if (json.at("box_depth").type() == nlohmann::detail::value_t::number_float)
        {
            boxDepth.value = json.value("box_depth", 1.0f);
        } else if (json.at("box_depth").type() == nlohmann::detail::value_t::string)
        {
            const std::string boxWidthValue = json.value("box_depth", "");
            if (boxWidthValue.starts_with("$"))
            {
                boxDepth.usesParam = true;
                boxDepth.paramName = boxWidthValue.substr(1, boxWidthValue.length() - 1);
            }
        }
    } else
    {
        boxDepth.value = 1.0f;
    }
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
            extents.x =  actor.params.at(boxWidth.paramName).Get<float>(1.0f);
        }
    } else
    {
        extents.x =  boxWidth.value;
    }

    if (boxHeight.usesParam)
    {
        if (actor.params.contains(boxHeight.paramName))
        {
            extents.y =  actor.params.at(boxHeight.paramName).Get<float>(1.0f);
        }
    } else
    {
        extents.y =  boxHeight.value;
    }

    if (boxDepth.usesParam)
    {
        if (actor.params.contains(boxDepth.paramName))
        {
            extents.z =  actor.params.at(boxDepth.paramName).Get<float>(1.0f);
        }
    } else
    {
        extents.z =  boxDepth.value;
    }

    return extents;
}
