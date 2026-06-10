//
// Created by droc101 on 6/9/26.
//

#pragma once


#include <cstddef>
#include <cstdint>
#include <libassets/type/ExpressionParser.h>
#include <libassets/type/Param.h>
#include <libassets/type/renderDefs/values/BasicDefinitionValue.h>
#include <libassets/util/Logger.h>
#include <regex>
#include <string>
#include <vector>

template<typename T> class NumericDefinitionValue: public BasicDefinitionValue<T>
{
    public:
        NumericDefinitionValue() = default;
        explicit NumericDefinitionValue(const float &value): BasicDefinitionValue<T>(value) {}
        NumericDefinitionValue(const nlohmann::json &json, const std::string &key, const T &defaultValue)
        {
            if (json.contains(key))
            {
                if (json.at(key).type() == nlohmann::detail::value_t::number_float ||
                    json.at(key).type() == nlohmann::detail::value_t::number_unsigned ||
                    json.at(key).type() == nlohmann::detail::value_t::number_integer)
                {
                    this->value = json.value(key, defaultValue);
                } else if (json.at(key).type() == nlohmann::detail::value_t::string)
                {
                    const std::string expr = json.value(key, "");
                    ParseExpression(expr, defaultValue);
                }
            } else
            {
                this->value = defaultValue;
            }
        }

        NumericDefinitionValue(const nlohmann::json &json, const size_t index, const T &defaultValue)
        {
            if (json.type() == nlohmann::detail::value_t::array && json.size() > index)
            {
                if (json.at(index).type() == nlohmann::detail::value_t::number_float ||
                    json.at(index).type() == nlohmann::detail::value_t::number_unsigned ||
                    json.at(index).type() == nlohmann::detail::value_t::number_integer)
                {
                    this->value = json.at(index);
                } else if (json.at(index).type() == nlohmann::detail::value_t::string)
                {
                    const std::string expr = json.at(index);
                    ParseExpression(expr, defaultValue);
                }
            } else
            {
                this->value = defaultValue;
            }
        }

        T Get(const KvList &params, const T &defaultValue) override
        {
            if (this->usesParam)
            {
                if (!expressionParser.IsReady())
                {
                    return defaultValue;
                }
                for (ExpressionVariable &i: varMetadata)
                {
                    expressionParser.SetVariable(i.original, ProcessExpressionVariable(i, params));
                }

                const double result = expressionParser.Evaluate();
                return static_cast<T>(result);
            }

            return this->value;
        }

    private:
        enum class VectorComponent : uint8_t
        {
            NONE,
            /// vec2 or vec3 X
            X,
            /// vec2 or vec3 Y
            Y,
            /// vec3 Z
            Z,
            /// Color Red
            R,
            /// Color Green
            G,
            /// Color Blue
            B,
            /// Color Alpha
            A
        };

        struct ExpressionVariable
        {
                /// The original text of this variable
                std::string original{};
                /// The extracted param name
                std::string paramName{};
                /// The vector component to use
                VectorComponent vectorComponent{};
        };

        ExpressionParser expressionParser{};
        std::vector<ExpressionVariable> varMetadata{};

        /**
         * Capture Groups: on "$param:x"
         * $param:x
         * param
         * x
         */
        static constexpr const char *PARAM_SEARCH_REGEX = R"/(\$([A-Za-z_]+)(?::([xyzrgba]))?)/";

        static T ProcessExpressionVariable(const ExpressionVariable &var, const KvList &params)
        {
            constexpr float DEFAULT_VALUE = 0;
            if (!params.contains(var.paramName))
            {
                return DEFAULT_VALUE;
            }
            if (var.vectorComponent != VectorComponent::NONE)
            {
                const Param &p = params.at(var.paramName);
                if (p.GetType() == Param::ParamType::PARAM_TYPE_VEC2)
                {
                    if (var.vectorComponent == VectorComponent::X)
                    {
                        return p.Get<glm::vec2>(glm::vec2{DEFAULT_VALUE}).x;
                    }
                    if (var.vectorComponent == VectorComponent::Y)
                    {
                        return p.Get<glm::vec2>(glm::vec2{DEFAULT_VALUE}).y;
                    }
                }
                if (p.GetType() == Param::ParamType::PARAM_TYPE_VEC3)
                {
                    if (var.vectorComponent == VectorComponent::X)
                    {
                        return p.Get<glm::vec3>(glm::vec3{DEFAULT_VALUE}).x;
                    }
                    if (var.vectorComponent == VectorComponent::Y)
                    {
                        return p.Get<glm::vec3>(glm::vec3{DEFAULT_VALUE}).y;
                    }
                    if (var.vectorComponent == VectorComponent::Z)
                    {
                        return p.Get<glm::vec3>(glm::vec3{DEFAULT_VALUE}).z;
                    }
                }
                if (p.GetType() == Param::ParamType::PARAM_TYPE_COLOR)
                {
                    if (var.vectorComponent == VectorComponent::R)
                    {
                        return p.Get<Color>(Color(-1)).R();
                    }
                    if (var.vectorComponent == VectorComponent::G)
                    {
                        return p.Get<Color>(Color(-1)).G();
                    }
                    if (var.vectorComponent == VectorComponent::B)
                    {
                        return p.Get<Color>(Color(-1)).B();
                    }
                    if (var.vectorComponent == VectorComponent::A)
                    {
                        return p.Get<Color>(Color(-1)).A();
                    }
                }
            }
            return params.at(var.paramName).Get(DEFAULT_VALUE);
        }

        void CompileExpression()
        {
            expressionParser.Compile();
        }

        void ParseExpression(const std::string &expression, T defaultValue)
        {
            this->usesParam = true;
            expressionParser = ExpressionParser(expression);
            std::smatch match;
            std::string::const_iterator iter = std::string::const_iterator(expression.cbegin());
            size_t matchNum = 0;
            while (std::regex_search(iter, expression.cend(), match, std::regex(PARAM_SEARCH_REGEX)))
            {
                if (match.length() < 2)
                {
                    Logger::Error("Incorrect number of matches on float render definition regex (string was \"{}\")",
                                  expression);
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
                    } else if (componentChar == 'r')
                    {
                        component = VectorComponent::R;
                    } else if (componentChar == 'g')
                    {
                        component = VectorComponent::G;
                    } else if (componentChar == 'b')
                    {
                        component = VectorComponent::B;
                    } else if (componentChar == 'a')
                    {
                        component = VectorComponent::A;
                    }
                }

                const ExpressionVariable var = {
                    .original = match[0].str(),
                    .paramName = match[1].str(),
                    .vectorComponent = component,
                };
                varMetadata.push_back(var);
                expressionParser.AddVariable(var.original);

                iter = match.suffix().first;
                matchNum++;
            }
            if (matchNum == 0)
            {
                Logger::Warning("No matches on string \"{}\"", expression);
            }

            CompileExpression();
            this->value = defaultValue;
        }
};
