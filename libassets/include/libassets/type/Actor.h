//
// Created by droc101 on 9/5/25.
//

#pragma once

#include <glm/vec3.hpp>
#include <libassets/type/ActorDefinition.h>
#include <libassets/type/IOConnection.h>
#include <libassets/type/Param.h>
#include <libassets/util/DataWriter.h>
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

        /**
         * Apply an ActorDefinition to this actor's params
         * @param definition The definition to apply
         * @param overwrite Whether to overwrite any existing params
         */
        void ApplyDefinition(const ActorDefinition &definition, bool overwrite);

        /**
         * Remove any params not specified in an actor definition
         * @param definition The definition to check against
         */
        void RemoveUnknownParams(const ActorDefinition &definition);

        nlohmann::ordered_json GenerateJson() const;

        void Write(DataWriter &writer) const;
};
