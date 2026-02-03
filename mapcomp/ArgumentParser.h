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

        [[nodiscard]] bool hasFlag(const std::string &flag) const;

        [[nodiscard]] bool hasFlagWithValue(const std::string &flag) const;

        [[nodiscard]] std::string getValue(const std::string &flag) const;

    private:
        std::vector<std::string> arguments{};
};
