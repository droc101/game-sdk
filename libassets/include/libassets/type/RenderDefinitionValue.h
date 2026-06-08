//
// Created by droc101 on 4/28/26.
//

#pragma once

#include <cmath>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <libassets/type/Color.h>
#include <libassets/type/ExpressionParser.h>
#include <libassets/type/Param.h>
#include <libassets/util/Logger.h>
#include <regex>
#include <string>
#include <vector>

template<typename T> concept RDVTypeTemplate = std::same_as<T, float> ||
                                               std::same_as<T, bool> ||
                                               std::same_as<T, std::string> ||
                                               std::same_as<T, Color> ||
                                               std::same_as<T, uint32_t>;

template<RDVTypeTemplate T> class RenderDefinitionValue
{
    public:
        RenderDefinitionValue() = default;
        explicit RenderDefinitionValue(const T &defaultValue)
        {
            usesParam = false;
            value = defaultValue;
        }
        RenderDefinitionValue(const nlohmann::json &json, const std::string &key, const T &defaultValue)
        {
            if constexpr (std::same_as<T, std::string>)
            {
                ConstructString(json, key, defaultValue);
            } else if constexpr (std::same_as<T, Color>)
            {
                ConstructColor(json, key, defaultValue);
            } else if constexpr (std::same_as<T, bool>)
            {
                ConstructBool(json, key, defaultValue);
            } else if constexpr (std::same_as<T, float>)
            {
                ConstructFloat(json, key, defaultValue);
            } else if constexpr (std::same_as<T, uint32_t>)
            {
                ConstructUint32(json, key, defaultValue);
            }
        }

        T Get(const KvList &params, const T &defaultValue) const
        {
            static_assert(!std::same_as<T, float>); // use GetFloat instead
            static_assert(!std::same_as<T, uint32_t>); // use GetInt instead
            if (usesParam)
            {
                if (params.contains(paramName))
                {
                    return params.at(paramName).Get<T>(defaultValue);
                }
            } else
            {
                return value;
            }
            return defaultValue;
        }

        float GetFloat(const KvList &params)
        {
            static_assert(std::same_as<T, float>);
            if (!expressionParser.IsReady() || !usesParam)
            {
                return value;
            }

            for (size_t i = 0; i < varMetadata.size(); i++)
            {
                expressionParser.SetVariable(varMetadata.at(i).original,
                                             ProcessExpressionVariable(varMetadata.at(i), params));
            }

            const double result = expressionParser.Evaluate();
            return static_cast<float>(result);
        }

        uint32_t GetInt(const KvList &params)
        {
            static_assert(std::same_as<T, uint32_t>);
            if (!expressionParser.IsReady() || !usesParam)
            {
                return value;
            }

            for (size_t i = 0; i < varMetadata.size(); i++)
            {
                expressionParser.SetVariable(varMetadata.at(i).original,
                                             ProcessExpressionVariable(varMetadata.at(i), params));
            }

            const double result = expressionParser.Evaluate();
            return static_cast<uint32_t>(std::round(result));
        }

    private:
        enum class VectorComponent : uint8_t
        {
            NONE,
            X,
            Y,
            Z
        };

        struct ExpressionVariable
        {
                std::string original{};
                std::string paramName{};
                VectorComponent vectorComponent{};
        };

        /**
         * Capture Groups: on "$param:x"
         * $param:x
         * param
         * x
         */
        static constexpr const char *PARAM_SEARCH_REGEX = R"/(\$([A-Za-z_]+)(?::([xyz]))?)/";

        bool usesParam = false;
        std::string paramName{};
        T value{};

        ExpressionParser expressionParser{};
        std::vector<ExpressionVariable> varMetadata{};

        void ConstructString(const nlohmann::json &json, const std::string &key, const std::string &defaultValue);
        void ConstructColor(const nlohmann::json &json, const std::string &key, const Color &defaultValue);
        void ConstructBool(const nlohmann::json &json, const std::string &key, const bool &defaultValue);
        void ConstructFloat(const nlohmann::json &json, const std::string &key, const float &defaultValue);
        void ConstructUint32(const nlohmann::json &json, const std::string &key, const uint32_t &defaultValue);

        T ProcessExpressionVariable(ExpressionVariable &var, const KvList &params)
        {
            static_assert(std::same_as<T, float> || std::same_as<T, uint32_t>);
            constexpr float DEFAULT_VALUE = 0;
            if (!params.contains(var.paramName))
            {
                return DEFAULT_VALUE;
            }
            if (var.vectorComponent != VectorComponent::NONE)
            {
                const Param &p = params.at(var.paramName);
                if (var.vectorComponent == VectorComponent::Z)
                {
                    return p.Get<glm::vec3>(glm::vec3{DEFAULT_VALUE}).z;
                }
                if (p.GetType() == Param::ParamType::PARAM_TYPE_VEC2)
                {
                    if (var.vectorComponent == VectorComponent::X)
                    {
                        return p.Get<glm::vec2>(glm::vec2{DEFAULT_VALUE}).x;
                    }
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
                    {
                        return p.Get<glm::vec3>(glm::vec3{DEFAULT_VALUE}).y;
                    }
                }
            }
            return params.at(var.paramName).Get(DEFAULT_VALUE);
        }

        void CompileExpression()
        {
            static_assert(std::same_as<T, float> || std::same_as<T, uint32_t>);
            expressionParser.Compile();
        }

        void ParseExpression(const std::string &expression, const T defaultValue)
        {
            usesParam = true;
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
            value = defaultValue;
        }
};
