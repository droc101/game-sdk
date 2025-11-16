//
// Created by droc101 on 9/5/25.
//

#pragma once

#include <cstddef>
#include <libassets/type/Param.h>
#include <libassets/util/DataReader.h>
#include <libassets/util/DataWriter.h>
#include <string>

class IOConnection
{
    public:
        IOConnection() = default;
        explicit IOConnection(const nlohmann::ordered_json &j);

        std::string targetName{};
        std::string sourceOutput{};
        std::string targetInput{};
        bool overridesParam = false;
        Param param{};
        size_t numRefires{};

        explicit IOConnection(DataReader &reader);

        void Write(DataWriter &writer) const;

        [[nodiscard]] nlohmann::ordered_json GenerateJson() const;
};
