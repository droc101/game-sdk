//
// Created by droc101 on 9/5/25.
//

#pragma once

#include <libassets/type/Param.h>
#include <libassets/util/DataReader.h>
#include <libassets/util/DataWriter.h>
#include <string>

class IOConnection
{
    public:
        std::string targetName{};
        std::string sourceOutput{};
        std::string targetInput{};
        bool overridesParam = false;
        Param param{};
        size_t numRefires{};

        IOConnection() = default;

        explicit IOConnection(DataReader &reader);

        void Write(DataWriter &writer) const;
};
