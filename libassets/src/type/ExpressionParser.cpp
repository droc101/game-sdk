//
// Created by droc101 on 6/7/26.
//

#include <cassert>
#include <cstddef>
#include <libassets/type/ExpressionParser.h>
#include <libassets/util/Logger.h>
#include <ranges>
#include <string>
#include <tinyexpr.h>
#include <utility>

ExpressionParser::ExpressionParser(const std::string &expression)
{
    this->expression = expression;
}

ExpressionParser::~ExpressionParser()
{
    if (expr != nullptr)
    {
        te_free(expr);
        expr = nullptr;
    }
}

ExpressionParser &ExpressionParser::operator=(const ExpressionParser &other)
{
    if (this == &other)
    {
        return *this;
    }
    expression = other.expression;
    varMetadata = other.varMetadata;
    vars.clear();
    for (const std::pair<const std::string, ExpressionVariable> &metadata: varMetadata)
    {
        vars.emplace_back(metadata.first.c_str(), &metadata.second.value, 0, nullptr);
    }

    expr = nullptr;
    if (other.expr != nullptr)
    {
        Compile();
    }
    return *this;
}

ExpressionParser::ExpressionParser(const ExpressionParser &other)
{
    *this = other;
}

void ExpressionParser::AddVariable(const std::string &variableName)
{
    assert(expr == nullptr);

    std::string compiledName = "v";
    size_t varIndex = varMetadata.size() + 1;
    while (varIndex > 0)
    {
        varIndex--;
        const char ltr = static_cast<char>('a' + (varIndex % 26));
        compiledName += ltr;
        varIndex /= 26;
    }

    const ExpressionVariable var = {
        .compiledName = compiledName,
        .value = 0,
    };
    varMetadata[variableName] = var;

    const std::string::size_type pos = expression.find(variableName);
    expression.replace(pos, variableName.length(), compiledName);
}

bool ExpressionParser::Compile()
{
    if (expr != nullptr)
    {
        Logger::Error("Expression is already compiled");
        return false;
    }

    for (const ExpressionVariable &val: varMetadata | std::views::values)
    {
        const te_variable var = {.name = val.compiledName.c_str(), .address = &val.value};
        vars.push_back(var);
    }

    int err = 0;
    te_expr *compiledExpr = te_compile(expression.c_str(), vars.data(), static_cast<int>(vars.size()), &err);
    if (compiledExpr == nullptr)
    {
        Logger::Error("Failed to parse expression \"{}\": parse error at {}", expression, err);
        return false;
    }
    expr = compiledExpr;
    return true;
}

void ExpressionParser::SetVariable(const std::string &variableName, const double value)
{
    assert(expr != nullptr);
    varMetadata.at(variableName).value = value;
}

double ExpressionParser::Evaluate() const
{
    assert(expr != nullptr);
    return te_eval(expr);
}

bool ExpressionParser::IsReady() const
{
    return expr != nullptr;
}
