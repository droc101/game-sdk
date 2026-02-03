//
// Created by droc101 on 9/5/25.
//

#pragma once

#include <glm/vec3.hpp>
#include <libassets/type/ActorDefinition.h>
#include <libassets/type/IOConnection.h>
#include <libassets/type/Param.h>
#include <string>
#include <vector>

class Actor
{
    public:
        Actor() = default;
        explicit Actor(nlohmann::ordered_json j);

        std::string className;
        KvList params{};
        std::vector<IOConnection> connections{};
        glm::vec3 position{};
        glm::vec3 rotation{};

        void ApplyDefinition(const ActorDefinition &definition, bool overwrite);

        nlohmann::ordered_json GenerateJson() const;

        void Write(DataWriter &writer) const;
};
