//
// Created by droc101 on 10/18/25.
//

#pragma once

#include <libassets/type/paramDefs/ParamDefinition.h>
#include <libassets/type/SignalDefinition.h>
#include <libassets/util/Error.h>
#include <string>
#include <unordered_map>
#include <unordered_set>

class ActorDefinition
{
    public:
        ActorDefinition() = default;
        ~ActorDefinition() = default; // TODO might have to delete param pointers

        std::string className;
        std::string description;

        std::string parentClassName;
        ActorDefinition *parentClass = nullptr;

        bool isVirtual = false;

        [[nodiscard]] static Error::ErrorCode Create(const std::string &path, ActorDefinition &definition);

        void GetInputNames(std::unordered_set<std::string> out) const;
        void GetOutputNames(std::unordered_set<std::string> out) const;
        void GetParamNames(std::unordered_set<std::string> out) const;

        [[nodiscard]] Error::ErrorCode GetInput(std::string &name, SignalDefinition &input) const;
        [[nodiscard]] Error::ErrorCode GetOutput(std::string &name, SignalDefinition &output) const;
        [[nodiscard]] Error::ErrorCode GetParam(std::string &name, ParamDefinition *param) const;

        std::unordered_map<std::string, SignalDefinition> inputs{};
        std::unordered_map<std::string, SignalDefinition> outputs{};
        std::unordered_map<std::string, ParamDefinition *> params{};
};
