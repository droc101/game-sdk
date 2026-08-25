//
// Created by droc101 on 11/17/25.
//

#include <algorithm>
#include <filesystem>
#include <libassets/util/ArgumentParser.h>
#include <string>

ArgumentParser::ArgumentParser(const int argc, const char **argv)
{
    for (int i = 0; i < argc; i++)
    {
        arguments.emplace_back(argv[i]);
    }
}

bool ArgumentParser::HasFlag(const std::string &flag) const
{
    return std::ranges::any_of(arguments, [&](const std::string &arg) { return arg == flag; });
}

bool ArgumentParser::HasFlagWithValue(const std::string &flag) const
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

std::string ArgumentParser::GetFlagValue(const std::string &flag) const
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

std::string ArgumentParser::GetFileArgument(const std::vector<std::string> &extensions) const
{
    for (const std::string &argument: arguments)
    {
        for (const std::string &extension: extensions)
        {
            if (argument.ends_with(extension) && std::filesystem::exists(argument))
            {
                return argument;
            }
        }
    }
    return "";
}
