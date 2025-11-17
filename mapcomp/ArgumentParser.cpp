//
// Created by droc101 on 11/17/25.
//

#include "ArgumentParser.h"

#include <algorithm>
#include <string>

ArgumentParser::ArgumentParser(const int argc, const char **argv)
{
    for (int i = 0; i < argc; i++)
    {
        arguments.emplace_back(argv[i]);
    }
}

bool ArgumentParser::hasFlag(const std::string &flag) const
{
    return std::ranges::any_of(arguments, [&](const std::string &arg) { return arg == flag; });
}

bool ArgumentParser::hasFlagWithValue(const std::string &flag) const
{
    const std::string prefix = flag + "=";
    for (const std::string &arg: arguments)
    {
        if (arg.starts_with(prefix))
        {
            return true;
        }
    }
    return false;
}

std::string ArgumentParser::getValue(const std::string &flag) const
{
    const std::string prefix = flag + "=";
    for (const std::string &arg: arguments)
    {
        if (arg.starts_with(prefix))
        {
            return arg.substr(prefix.size());
        }
    }
    return "";
}
