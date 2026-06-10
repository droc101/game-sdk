//
// Created by droc101 on 10/18/25.
//

#pragma once

#include <libassets/type/paramDefs/ParamDefinition.h>
#include <libassets/type/renderDefs/RenderDefinition.h>
#include <libassets/type/SignalDefinition.h>
#include <libassets/util/Error.h>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class ActorDefinition
{
    public:
        ActorDefinition() = default;

        std::string className;
        std::string description;

        std::string parentClassName;
        ActorDefinition *parentClass = nullptr;

        bool isVirtual = false;

        std::vector<std::shared_ptr<RenderDefinition>> renderDefinitions{};

        std::unordered_map<std::string, SignalDefinition> inputs{};
        std::unordered_map<std::string, SignalDefinition> outputs{};
        std::unordered_map<std::string, std::shared_ptr<ParamDefinition>> params{};

        static constexpr const char *VALID_ACTOR_DEFINITION_IDENTIFIER_REGEX = R"/(^[a-z_]+$)/";

        /**
         * Create an ActorDefinition from a JSON file
         * @param path The path to the JSON file
         * @param definition The definition to populate
         */
        [[nodiscard]] static Error::ErrorCode Create(const std::string &path, ActorDefinition &definition);

        /**
         * Check if this definition extends another at any point
         * @param baseClass The class to look for
         */
        [[nodiscard]] bool Extends(const std::string &baseClass) const;

        /**
         * Get all input names in this definition and all parents
         */
        void GetInputNames(std::unordered_set<std::string> &out) const;
        /**
         * Get all output names in this definition and all parents
         */
        void GetOutputNames(std::unordered_set<std::string> &out) const;
        /**
         * Get all param names in this definition and all parents
         */
        void GetParamNames(std::unordered_set<std::string> &out) const;

        /**
         * Get an input definition by name
         * @param name The input name
         * @param input The SignalDefinition to populate
         */
        [[nodiscard]] Error::ErrorCode GetInput(const std::string &name, SignalDefinition &input) const;
        /**
         * Get an output definition by name
         * @param name The output name
         * @param output The SignalDefinition to populate
         */
        [[nodiscard]] Error::ErrorCode GetOutput(const std::string &name, SignalDefinition &output) const;
        /**
         * Get a param definition by name
         * @param name The param name
         * @param param The ParamDefinition to populate
         */
        [[nodiscard]] Error::ErrorCode GetParam(const std::string &name, std::shared_ptr<ParamDefinition> &param) const;
};
