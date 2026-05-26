//
// Created by droc101 on 4/28/26.
//

#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <libassets/type/Color.h>
#include <libassets/type/Param.h>
#include <libassets/util/Logger.h>
#include <string>
#include <tinyexpr.h>
#include <vector>

template<typename T> concept RDVTypeTemplate = std::same_as<T, float> ||
                                               std::same_as<T, bool> ||
                                               std::same_as<T, std::string> ||
                                               std::same_as<T, Color>;

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
            }
        }

        ~RenderDefinitionValue()
        {
            if (expr != nullptr)
            {
                te_free(expr);
                expr = nullptr;
            }
        }

        RenderDefinitionValue &operator=(const RenderDefinitionValue &other)
        {
            if (this == &other)
            {
                return *this;
            }
            usesParam = other.usesParam;
            paramName = other.paramName;
            value = other.value;
            expression = other.expression;
            varMetadata = other.varMetadata;
            if constexpr (std::same_as<T, float>)
            {
                vars.clear();
                for (const ExpressionVariable &metadata: varMetadata)
                {
                    vars.emplace_back(metadata.variableName.c_str(), &metadata.value, 0, nullptr);
                }
                expr = nullptr;
                if (other.expr != nullptr)
                {
                    CompileExpression();
                }
            }
            return *this;
        }
        RenderDefinitionValue(const RenderDefinitionValue &other)
        {
            *this = other;
        }

        T Get(const KvList &params, const T &defaultValue) const
        {
            static_assert(!std::same_as<T, float>); // use GetFloat instead
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
            if (expr == nullptr || !usesParam)
            {
                return value;
            }

            for (size_t i = 0; i < varMetadata.size(); i++)
            {
                ProcessExpressionVariable(varMetadata.at(i), params);
                vars.at(i).address = &varMetadata.at(i).value;
            }

            const double result = te_eval(expr);
            return static_cast<float>(result);
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
                std::string variableName{};
                std::string original{};
                std::string paramName{};
                VectorComponent vectorComponent{};
                double value{};
        };

        /**
         * Capture Groups: on "$param:x"
         * $param:x
         * param
         * x
         */
        static constexpr const char *PARAM_SEARCH_REGEX = R"/(\$([a-z_]+)(?::([xyz]))?)/";

        bool usesParam = false;
        std::string paramName{};
        T value{};

        std::string expression{};
        te_expr *expr = nullptr;
        std::vector<ExpressionVariable> varMetadata{};
        std::vector<te_variable> vars{};

        void ConstructString(const nlohmann::json &json, const std::string &key, const std::string &defaultValue);
        void ConstructColor(const nlohmann::json &json, const std::string &key, const Color &defaultValue);
        void ConstructBool(const nlohmann::json &json, const std::string &key, const bool &defaultValue);
        void ConstructFloat(const nlohmann::json &json, const std::string &key, const float &defaultValue);

        void ProcessExpressionVariable(ExpressionVariable &var, const KvList &params);

        void CompileExpression()
        {
            static_assert(std::same_as<T, float>);
            int err = 0;
            te_expr *compiledExpr = te_compile(expression.c_str(), vars.data(), static_cast<int>(vars.size()), &err);
            if (compiledExpr == nullptr)
            {
                Logger::Error("Failed to parse expression \"{}\": parse error at {}", expression, err);
            } else
            {
                expr = compiledExpr;
            }
        }
};
