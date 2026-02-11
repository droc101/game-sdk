//
// Created by droc101 on 11/17/25.
//

#pragma once

#include <cstdint>
#include <SDL3/SDL_process.h>
#include <SDL3/SDL_timer.h>
#include <string>
#include <vector>

class DesktopInterface
{
    public:
        static DesktopInterface &Get();

        /**
         * Execute a process, blocking until completion
         * @param executable Executable to run
         * @param arguments Arguments to pass to the process
         * @param exitCode Where to store the process's exit code, can be nullptr
         */
        bool ExecuteProcess(const std::string &executable,
                                   const std::vector<std::string> &arguments,
                                   int *exitCode);

        /**
         * Create and start an SDL_Process
         * @param executable The executable to run
         * @param arguments Arguments to pass to the process
         * @return SDL_Process pointer or nullptr on failure
         */
        SDL_Process *StartSDLProcess(const std::string &executable, const std::vector<std::string> &arguments);

        /**
         * Execute a process, do not block
         * @param executable Executable to run
         * @param arguments Arguments to pass to the process
         * @note You cannot later block on this process or obtain its exit code
         */
        bool ExecuteProcessNonBlocking(const std::string &executable, const std::vector<std::string> &arguments);

        /**
         * Open a filesystem path in the default program
         * @param path The path to open
         */
        bool OpenFilesystemPath(const std::string &path);

        /**
         * Open a URL in the default program
         * @param url The URL to open
         */
        bool OpenURL(const std::string &url);

        /**
         * Get a file path argument
         * @param argc main function argc
         * @param argv main function argv
         * @param extensions list of accepted extensions
         * @return file path on success, empty string otherwise
         */
        std::string GetFileArgument(int argc, char **argv, const std::vector<std::string> &extensions);

        /**
         * Initialize the desktop interface
         */
        void InitDesktopInterface();

    private:
        std::vector<SDL_Process *> processes{};
        SDL_TimerID gcTimer{};

        DesktopInterface() = default;

        static uint32_t GarbageCollectorCallback(void *userdata, SDL_TimerID timer, uint32_t interval);
};
