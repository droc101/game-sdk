//
// Created by droc101 on 6/9/26.
//

#include <libassets/type/Color.h>
#include <libassets/type/Param.h>
#include <libassets/type/renderDefs/values/ColorDefinitionValue.h>
#include <libassets/type/renderDefs/values/NumericDefinitionValue.h>
#include <string>

ColorDefinitionValue::ColorDefinitionValue(const Color &color)
{
    SetValue(color);
    usesParam = false;
}

ColorDefinitionValue::ColorDefinitionValue(const nlohmann::json &json,
                                           const std::string &key,
                                           const Color &defaultValue)
{
    if (json.contains(key))
    {
        if (json.at(key).type() == nlohmann::detail::value_t::number_unsigned ||
            json.at(key).type() == nlohmann::detail::value_t::number_integer)
        {
            SetValue(Color(json.value(key, -1u)));
        } else if (json.at(key).type() == nlohmann::detail::value_t::string)
        {
            const std::string colorValue = json.value(key, "");
            if (colorValue.starts_with("$"))
            {
                usesParam = true;
                paramName = colorValue.substr(1, colorValue.length() - 1);
            } else if (colorValue.starts_with("#"))
            {
                SetValue(Color(colorValue));
            } else
            {
                SetValue(defaultValue);
            }
        } else if (json.at(key).type() == nlohmann::detail::value_t::array)
        {
            const nlohmann::json &arr = json.at(key);
            r = NumericDefinitionValue<float>(arr, 0, 1.0f);
            g = NumericDefinitionValue<float>(arr, 1, 1.0f);
            b = NumericDefinitionValue<float>(arr, 2, 1.0f);
            a = NumericDefinitionValue<float>(arr, 3, 1.0f);
        }
    } else
    {
        SetValue(defaultValue);
    }
}

void ColorDefinitionValue::SetValue(const Color &color)
{
    r = NumericDefinitionValue<float>(color.R());
    g = NumericDefinitionValue<float>(color.G());
    b = NumericDefinitionValue<float>(color.B());
    a = NumericDefinitionValue<float>(color.A());
}

Color ColorDefinitionValue::Get(const KvList &params, const Color &defaultValue)
{
    if (usesParam)
    {
        if (params.contains(paramName))
        {
            return params.at(paramName).Get<Color>(defaultValue);
        }
    } else
    {
        return Color(r.Get(params, defaultValue.R()),
                     g.Get(params, defaultValue.G()),
                     b.Get(params, defaultValue.B()),
                     a.Get(params, defaultValue.A()));
    }
    return defaultValue;
}
