//
// Created by droc101 on 9/5/25.
//

#pragma once
#include <string>
#include "Param.h"
#include "libassets/util/DataWriter.h"


class IOConnection
{
    public:
        std::string targetName;
        std::string sourceOutput;
        std::string targetInput;
        Param param;

        IOConnection() = default;

        void Write(DataWriter &writer) const;
};
