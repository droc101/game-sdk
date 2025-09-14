//
// Created by droc101 on 9/5/25.
//

#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include "IOConnection.h"
#include "Param.h"


class Actor
{
    public:
        std::string typeName;
        std::string name;
        std::unordered_map<std::string, Param> params;
        std::vector<IOConnection> connections;
};
