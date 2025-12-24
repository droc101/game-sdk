//
// Created by droc101 on 11/17/25.
//

#pragma once

#include <SDL3/SDL_process.h>
#include <SDL3/SDL_timer.h>
#include <cstdint>
#include <string>
#include <vector>

class DesktopInterface
{
    public:
        DesktopInterface() = delete;

        static bool ExecuteProcess(const std::string &executable,
                                   const std::vector<std::string> &arguments,
                                   int *exitCode);

        static bool ExecuteProcessNonBlocking(const std::string &executable, const std::vector<std::string> &arguments);

        static bool OpenFilesystemPath(const std::string &path);

        static bool OpenURL(const std::string &url);

        static void InitDesktopInterface();

    private:
        static inline std::vector<SDL_Process *> processes{};
        static inline SDL_TimerID gcTimer;

        static uint32_t GarbageCollectorCallback(void *userdata, SDL_TimerID timer, uint32_t interval);
};
