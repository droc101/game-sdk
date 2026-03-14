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
        explicit ActorDefinitionManager(const SearchPathManager &searchPathManager);

        [[nodiscard]] bool HasActorClass(const std::string &className) const;

        [[nodiscard]] const ActorDefinition &GetActorDefinition(const std::string &className) const;

        [[nodiscard]] size_t GetActorClassCount() const;

        [[nodiscard]] const std::vector<std::string> &GetActorClasses() const;

    private:
        std::map<std::string, OptionDefinition> optionDefinitions{};
        std::map<std::string, ActorDefinition> actorDefinitions{};
        std::vector<std::string> actorClassNames{};

        void LoadActorDefinitions(const SearchPathManager &pathManager);
        void LoadOptionDefinitions(const SearchPathManager &pathManager);
};


#endif //GAME_SDK_ACTORDEFINITIONMANAGER_H
