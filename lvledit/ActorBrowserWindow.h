//
// Created by droc101 on 10/19/25.
//

#pragma once

#include <cstddef>
#include <libassets/type/ActorDefinition.h>
#include <string>

class ActorBrowserWindow
{
    public:
        ActorBrowserWindow() = delete;

        static inline bool visible = false;

        static void Render();

    private:
        static inline std::string selectedClass = "actor";
        static inline size_t selectedParam = 0;

        static void RenderParamsTab(const ActorDefinition &def);

        static void RenderInputsTab(const ActorDefinition &def);

        static void RenderOutputsTab(const ActorDefinition &def);
};
