//
// Created by droc101 on 3/12/26.
//

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <libassets/type/ActorDefinition.h>
#include <libassets/type/OptionDefinition.h>
#include <libassets/type/paramDefs/OptionParamDefinition.h>
#include <libassets/type/paramDefs/ParamDefinition.h>
#include <libassets/util/ActorDefinitionManager.h>
#include <libassets/util/Error.h>
#include <libassets/util/Logger.h>
#include <libassets/util/SearchPathManager.h>
#include <memory>
#include <ranges>
#include <string>
#include <utility>
#include <vector>

ActorDefinitionManager::ActorDefinitionManager(const SearchPathManager &searchPathManager, Error &status)
{
    LoadOptionDefinitions(searchPathManager);
    status = LoadActorDefinitions(searchPathManager);
}

void ActorDefinitionManager::LoadOptionDefinitions(const SearchPathManager &pathManager)
{
    const std::vector<std::string> defs = pathManager.ScanAssetFolderA("defs/options", ".json");
    for (const std::string &val: defs)
    {
        OptionDefinition def{};
        const Error e = OptionDefinition::Create(val, def);
        if (e.Ok())
        {
            optionDefinitions[def.GetName()] = def;
        } else
        {
            Logger::Error("Failed to load option def {}: {}", val.c_str(), e);
        }
    }
    Logger::Verbose("Loaded {} option definitions", optionDefinitions.size());
}

Error ActorDefinitionManager::LoadActorDefinitions(const SearchPathManager &pathManager)
{
    const std::vector<std::string> defs = pathManager.ScanAssetFolderA("defs/actors", ".json");
    for (const std::string &val: defs)
    {
        ActorDefinition def{};
        const Error e = ActorDefinition::Create(val, def);
        if (e.Ok())
        {
            actorDefinitions[def.className] = def;
            actorClassNames.push_back(def.className);
        } else
        {
            Logger::Error("Failed to load actor def {}: {}", val.c_str(), e);
        }
    }

    for (std::pair<const std::string, ActorDefinition> &def: actorDefinitions)
    {
        if (!def.second.parentClassName.empty())
        {
            if (def.second.parentClassName == def.first)
            {
                Logger::Error("An actor class cannot inherit from itself!");
                return Errors::INCORRECT_FORMAT;
            }
            def.second.parentClass = &actorDefinitions.at(def.second.parentClassName);
        }

        for (const std::shared_ptr<ParamDefinition> &val: def.second.params | std::views::values)
        {
            OptionParamDefinition *opt = dynamic_cast<OptionParamDefinition *>(val.get());
            if (opt != nullptr)
            {
                opt->definition = &optionDefinitions.at(opt->optionListName);
                if (opt->definition == nullptr)
                {
                    Logger::Error("Failed to find option definition \"{}\"", opt->optionListName);
                    return Errors::FILE_NOT_FOUND;
                }
            }
        }
    }

    Logger::Verbose("Loaded {} actor definitions", actorDefinitions.size());

    if (!actorDefinitions.contains("player"))
    {
        Logger::Warning("No \"player\" actor class definition was loaded!");
    }
    if (!actorDefinitions.contains("actor"))
    {
        Logger::Warning("No \"actor\" actor class definition was loaded!");
    }

    std::ranges::sort(actorClassNames, [](const std::string &a, const std::string &b) { return a < b; });

    return Errors::OK;
}

bool ActorDefinitionManager::HasActorClass(const std::string &className) const
{
    return actorDefinitions.contains(className);
}

const ActorDefinition &ActorDefinitionManager::GetActorDefinition(const std::string &className) const
{
    assert(HasActorClass(className));
    return actorDefinitions.at(className);
}

size_t ActorDefinitionManager::GetActorClassCount() const
{
    return actorDefinitions.size();
}

const std::vector<std::string> &ActorDefinitionManager::GetActorClasses() const
{
    return actorClassNames;
}

void ActorDefinitionManager::GetDerivedActorClasses(const std::string &baseClassName,
                                                    std::vector<std::string> &derivedClassOutput) const
{
    for (const ActorDefinition &actorDefinition: actorDefinitions | std::views::values)
    {
        if (actorDefinition.parentClassName == baseClassName)
        {
            derivedClassOutput.push_back(actorDefinition.className);
        }
    }
}
