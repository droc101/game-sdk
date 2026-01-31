//
// Created by droc101 on 9/5/25.
//

#pragma once

#include <array>
#include <libassets/type/ActorDefinition.h>
#include <libassets/type/IOConnection.h>
#include <libassets/type/Param.h>
#include <string>
#include <unordered_map>
#include <vector>

class Actor
{
    public:
        Actor() = default;
        explicit Actor(nlohmann::ordered_json j);

        std::string className;
        KvList params{};
        std::vector<IOConnection> connections{};
        std::array<float, 3> position{};
        std::array<float, 3> rotation{};

        void ApplyDefinition(const ActorDefinition &definition, bool overwrite);

        nlohmann::ordered_json GenerateJson() const;

        void Write(DataWriter &writer) const;
};
