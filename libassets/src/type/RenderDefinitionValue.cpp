//
// Created by droc101 on 5/25/26.
//

#include <cstddef>
#include <libassets/type/Color.h>
#include <libassets/type/Param.h>
#include <libassets/type/RenderDefinitionValue.h>
#include <libassets/util/Logger.h>
#include <regex>
#include <string>
#include <tinyexpr.h>

template<> void RenderDefinitionValue<std::string>::ConstructString(const nlohmann::json &json,
                                                                    const std::string &key,
                                                                    const std::string &defaultValue)
{
    if (json.contains(key) && json.at(key).type() == nlohmann::detail::value_t::string)
    {
        const std::string jsonValue = json.value(key, defaultValue);
        if (jsonValue.starts_with("$"))
        {
            usesParam = true;
            paramName = jsonValue.substr(1, jsonValue.length() - 1);
        } else
        {
            value = jsonValue;
        }
    } else
    {
        value = defaultValue;
    }
}

template<> void RenderDefinitionValue<Color>::ConstructColor(const nlohmann::json &json,
                                                             const std::string &key,
                                                             const Color &defaultValue)
{
    if (json.contains(key))
    {
        if (json.at(key).type() == nlohmann::detail::value_t::number_unsigned ||
            json.at(key).type() == nlohmann::detail::value_t::number_integer)
        {
            value = Color(json.value(key, -1u));
        } else if (json.at(key).type() == nlohmann::detail::value_t::string)
        {
            const std::string colorValue = json.value(key, "");
            if (colorValue.starts_with("$"))
            {
                usesParam = true;
                paramName = colorValue.substr(1, colorValue.length() - 1);
            } else
            {
                value = defaultValue;
            }
        }
    } else
    {
        value = defaultValue;
    }
}

template<> void RenderDefinitionValue<bool>::ConstructBool(const nlohmann::json &json,
                                                           const std::string &key,
                                                           const bool &defaultValue)
{
    if (json.contains(key))
    {
        if (json.at(key).type() == nlohmann::detail::value_t::boolean)
        {
            value = json.value(key, defaultValue);
        } else if (json.at(key).type() == nlohmann::detail::value_t::string)
        {
            const std::string boolValue = json.value(key, "");
            if (boolValue.starts_with("$"))
            {
                usesParam = true;
                paramName = boolValue.substr(1, boolValue.length() - 1);
            } else
            {
                value = defaultValue;
            }
        }
    } else
    {
        value = defaultValue;
    }
}

template<> void RenderDefinitionValue<float>::ConstructFloat(const nlohmann::json &json,
                                                             const std::string &key,
                                                             const float &defaultValue)
{
    if (json.contains(key))
    {
        if (json.at(key).type() == nlohmann::detail::value_t::number_float ||
            json.at(key).type() == nlohmann::detail::value_t::number_unsigned ||
            json.at(key).type() == nlohmann::detail::value_t::number_integer)
        {
            value = json.value(key, defaultValue);
        } else if (json.at(key).type() == nlohmann::detail::value_t::string)
        {
            usesParam = true;
            const std::string floatValue = json.value(key, "");
            std::smatch match;
            std::string::const_iterator iter = std::string::const_iterator(floatValue.cbegin());
            size_t matchNum = 0;
            while (std::regex_search(iter, floatValue.cend(), match, std::regex(PARAM_SEARCH_REGEX)))
            {
                if (match.length() < 2)
                {
                    Logger::Error("Incorrect number of matches on float render definition regex (string was \"{}\")",
                                  floatValue);
                }
                VectorComponent component = VectorComponent::NONE;
                if (match.length() >= 3 && !match[2].str().empty())
                {
                    const char componentChar = match[2].str()[0];
                    if (componentChar == 'x')
                    {
                        component = VectorComponent::X;
                    } else if (componentChar == 'y')
                    {
                        component = VectorComponent::Y;
                    } else if (componentChar == 'z')
                    {
                        component = VectorComponent::Z;
                    }
                }

                std::string expressionVarName = "p";
                size_t varIndex = matchNum + 1;
                while (varIndex > 0)
                {
                    varIndex--;
                    const char ltr = static_cast<char>('a' + (varIndex % 26));
                    expressionVarName += ltr;
                    varIndex /= 26;
                }

                const ExpressionVariable var = {
                    .variableName = expressionVarName,
                    .original = match[0].str(),
                    .paramName = match[1].str(),
                    .vectorComponent = component,
                    .value = 0,
                };
                varMetadata.push_back(var);

                iter = match.suffix().first;
                matchNum++;
            }
            if (matchNum == 0)
            {
                Logger::Warning("No matches on string \"{}\"", floatValue);
            }

            // After this point **DO NOT MODIFY var_metadata**

            expression = floatValue;

            for (const ExpressionVariable &component : varMetadata)
            {
                const std::string::size_type pos = expression.find(component.original);
                expression.replace(pos, component.original.length(), component.variableName);
            }

            for (const ExpressionVariable &component : varMetadata)
            {
                const te_variable var = {.name = component.variableName.c_str(), .address = &component.value};
                vars.push_back(var);
            }

            CompileExpression();
            value = defaultValue;
        }
    } else
    {
        value = defaultValue;
    }
}

template<> void RenderDefinitionValue<float>::ProcessExpressionVariable(ExpressionVariable &var, const KvList &params)
{
    constexpr float DEFAULT_VALUE = 0;
    if (!params.contains(var.paramName))
    {
        var.value = DEFAULT_VALUE;
        return;
    }
    if (var.vectorComponent != VectorComponent::NONE)
    {
        const Param &p = params.at(var.paramName);
        if (var.vectorComponent == VectorComponent::Z)
        {
            var.value = p.Get<glm::vec3>(glm::vec3{DEFAULT_VALUE}).z;
            return;
        }
        if (p.GetType() == Param::ParamType::PARAM_TYPE_VEC2)
        {
            if (var.vectorComponent == VectorComponent::X)
            {
                var.value = p.Get<glm::vec2>(glm::vec2{DEFAULT_VALUE}).x;
                return;
            }
            {
                var.value = p.Get<glm::vec2>(glm::vec2{DEFAULT_VALUE}).y;
                return;
            }
        }
        if (p.GetType() == Param::ParamType::PARAM_TYPE_VEC3)
        {
            if (var.vectorComponent == VectorComponent::X)
            {
                var.value = p.Get<glm::vec3>(glm::vec3{DEFAULT_VALUE}).x;
                return;
            }
            {
                var.value = p.Get<glm::vec3>(glm::vec3{DEFAULT_VALUE}).y;
                return;
            }
        }
    }
    var.value = params.at(var.paramName).Get<float>(DEFAULT_VALUE);
}
