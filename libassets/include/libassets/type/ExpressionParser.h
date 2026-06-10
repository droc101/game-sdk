//
// Created by droc101 on 6/7/26.
//

#pragma once

#include <string>
#include <tinyexpr.h>
#include <unordered_map>
#include <vector>

class ExpressionParser
{
    public:
        ExpressionParser() = default;
        explicit ExpressionParser(const std::string &expression);
        ~ExpressionParser();
        ExpressionParser &operator=(const ExpressionParser &other);
        ExpressionParser(const ExpressionParser &other);

        /**
         * Add a variable to this ExpressionParser
         * @param variableName The name of the variable
         * @note This name will change in the compiled expression
         * @note This expression must not be compiled before calling this method
         */
        void AddVariable(const std::string &variableName);

        /**
         * Compile this expression
         * @note This expression must not be compiled before calling this method
         */
        bool Compile();

        /**
         * Set a variable's value
         * @param variableName The variable to set
         * @param value The value to assign
         * @note This expression must be compiled before calling this method
         */
        void SetVariable(const std::string &variableName, double value);

        /**
         * Evaluate this expression
         * @note This expression must be compiled before calling this method
         */
        double Evaluate() const;

        /**
         * Check if this expression is compiled
         */
        bool IsReady() const;

    private:

        struct ExpressionVariable
        {
                /// The compiled name of this variable
                std::string compiledName{};
                /// The value assigned to this variable
                double value{};
        };

        std::string expression{};
        te_expr *expr = nullptr;
        std::unordered_map<std::string, ExpressionVariable> varMetadata{};
        std::vector<te_variable> vars{};

        static double LightFalloffFunction(double constant,
                                           double linear,
                                           double quadratic,
                                           double scale,
                                           double percent);
};
