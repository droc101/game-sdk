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
