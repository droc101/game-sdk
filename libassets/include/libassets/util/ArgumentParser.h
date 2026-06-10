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

        /**
         * Check if a flag has been passed without a value
         */
        [[nodiscard]] bool HasFlag(const std::string &flag) const;

        /**
         * Check if a flag has been passed with a value
         */
        [[nodiscard]] bool HasFlagWithValue(const std::string &flag) const;

        /**
         * Get the value of a flag
         */
        [[nodiscard]] std::string GetFlagValue(const std::string &flag) const;

    private:
        std::vector<std::string> arguments{};
};
