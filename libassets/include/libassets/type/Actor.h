//
// Created by droc101 on 9/5/25.
//

#pragma once

#include <libassets/type/IOConnection.h>
#include <libassets/type/Param.h>
#include <string>
#include <unordered_map>
#include <vector>

class Actor
{
    public:
        std::string typeName;
        std::string name;
        std::unordered_map<std::string, Param> params;
        std::vector<IOConnection> connections;
};
