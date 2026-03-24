//
// Created by droc101 on 11/17/25.
//

#pragma once

#include <string>
#include <vector>

class ArgumentParser
{
    public:
        ArgumentParser(int argc, const char **argv);

        [[nodiscard]] bool HasFlag(const std::string &flag) const;

        [[nodiscard]] bool HasFlagWithValue(const std::string &flag) const;

        [[nodiscard]] std::string GetFlagValue(const std::string &flag) const;

    private:
        std::vector<std::string> arguments{};
};
