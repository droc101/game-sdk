//
// Created by droc101 on 11/17/25.
//

#include "DesktopInterface.h"

#include <SDL3/SDL_timer.h>
#include <cstdint>
#include <filesystem>
#include <SDL3/SDL_misc.h>
#include <SDL3/SDL_process.h>
#include <string>
#include <utility>
#include <vector>

#ifdef WIN32
#include <windows.h>
#include <shellapi.h>
#endif

bool DesktopInterface::ExecuteProcess(const std::string &executable,
                                      const std::vector<std::string> &arguments,
                                      int *exitCode)
{
    std::vector<const char *> args{};
    args.push_back(executable.c_str());
    for (const std::string &argument: arguments)
    {
        args.push_back(argument.c_str());
    }
    args.push_back(nullptr);

    SDL_Process *p = SDL_CreateProcess(args.data(), false);
    if (p == nullptr)
    {
        return false;
    }
    return SDL_WaitProcess(p, true, exitCode);
}

bool DesktopInterface::ExecuteProcessNonBlocking(const std::string &executable,
                                                 const std::vector<std::string> &arguments)
{
    std::vector<const char *> args{};
    args.push_back(executable.c_str());
    for (const std::string &argument: arguments)
    {
        args.push_back(argument.c_str());
    }
    args.push_back(nullptr);

    SDL_Process *p = SDL_CreateProcess(args.data(), false);
    if (p == nullptr)
    {
        return false;
    }
    processes.push_back(p);
    return true;
}


bool DesktopInterface::OpenURL(const std::string &url)
{
    return SDL_OpenURL(url.c_str());
}

bool DesktopInterface::OpenFilesystemPath(const std::string &path)
{
#ifdef WIN32
    std::string operation = "open";
    if (std::filesystem::is_directory(path))
    {
        operation = "explore";
    }
    SHELLEXECUTEINFO info{};
    info.cbSize = sizeof(info);
    info.lpVerb = operation.c_str();
    info.lpFile = path.c_str();
    info.nShow = SW_SHOW;
    info.fMask = 0;
    return ShellExecuteEx(&info);
#else
    return ExecuteProcess("xdg-open", {path}, nullptr);
#endif
}

uint32_t DesktopInterface::GarbageCollectorCallback(void *userdata, SDL_TimerID timer, uint32_t interval)
{
    std::vector<SDL_Process *>::iterator iter = processes.begin();
    while (iter != processes.end())
    {
        SDL_Process *p = *iter.base();
        if (SDL_WaitProcess(p, false, nullptr))
        {
            SDL_DestroyProcess(p);
            iter = processes.erase(iter);
        } else
        {
            iter += 1;
        }
    }
    return interval;
}

void DesktopInterface::InitDesktopInterface()
{
    gcTimer = SDL_AddTimer(1000, GarbageCollectorCallback, nullptr);
}
