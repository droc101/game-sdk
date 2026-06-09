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

        void AddVariable(const std::string &variableName);

        bool Compile();

        void SetVariable(const std::string &variableName, double value);

        double Evaluate() const;

        bool IsReady() const;

    private:

        struct ExpressionVariable
        {
                std::string compiledName{};
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
