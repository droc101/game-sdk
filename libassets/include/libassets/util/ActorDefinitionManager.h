//
// Created by droc101 on 3/12/26.
//

#ifndef GAME_SDK_ACTORDEFINITIONMANAGER_H
#define GAME_SDK_ACTORDEFINITIONMANAGER_H

#include <cstddef>
#include <libassets/type/ActorDefinition.h>
#include <libassets/type/OptionDefinition.h>
#include <libassets/util/SearchPathManager.h>
#include <map>
#include <string>
#include <vector>

class ActorDefinitionManager
{
    public:
        ActorDefinitionManager() = default;
        ActorDefinitionManager(const SearchPathManager &searchPathManager, Error::ErrorCode &status);

        /**
         * Check if a given actor class exists
         */
        [[nodiscard]] bool HasActorClass(const std::string &className) const;

        /**
         * Get the definition of a given actor class
         */
        [[nodiscard]] const ActorDefinition &GetActorDefinition(const std::string &className) const;

        /**
         * Get the number of actor classes loaded
         */
        [[nodiscard]] size_t GetActorClassCount() const;

        /**
         * Get all loaded actor classes
         */
        [[nodiscard]] const std::vector<std::string> &GetActorClasses() const;

    private:
        std::map<std::string, OptionDefinition> optionDefinitions{};
        std::map<std::string, ActorDefinition> actorDefinitions{};
        std::vector<std::string> actorClassNames{};

        Error::ErrorCode LoadActorDefinitions(const SearchPathManager &pathManager);
        void LoadOptionDefinitions(const SearchPathManager &pathManager);
};


#endif //GAME_SDK_ACTORDEFINITIONMANAGER_H
