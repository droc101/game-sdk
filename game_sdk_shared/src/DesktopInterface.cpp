//
// Created by droc101 on 11/17/25.
//

#include <cstdint>
#include <filesystem>
#include <game_sdk/DesktopInterface.h>
#include <SDL3/SDL_misc.h>
#include <SDL3/SDL_process.h>
#include <SDL3/SDL_timer.h>
#include <string>
#include <utility>
#include <vector>

#ifdef WIN32
// clang-format off
#include <windows.h>
#include <shellapi.h>
// clang-format on
#endif

DesktopInterface &DesktopInterface::Get()
{
    static DesktopInterface desktopInterfaceSingleton{};

    return desktopInterfaceSingleton;
}

bool DesktopInterface::ExecuteProcess(const std::string &executable,
                                      const std::vector<std::string> &arguments,
                                      int *exitCode)
{
    SDL_Process *p = StartSDLProcess(executable, arguments);
    if (p == nullptr)
    {
        return false;
    }
    return SDL_WaitProcess(p, true, exitCode);
}

SDL_Process *DesktopInterface::StartSDLProcess(const std::string &executable, const std::vector<std::string> &arguments)
{
    std::vector<const char *> args{};
    args.push_back(executable.c_str());
    for (const std::string &argument: arguments)
    {
        args.push_back(argument.c_str());
    }
    args.push_back(nullptr);

    const SDL_PropertiesID props = SDL_CreateProperties();
    (void)SDL_SetPointerProperty(props, SDL_PROP_PROCESS_CREATE_ARGS_POINTER, reinterpret_cast<void *>(args.data()));
    (void)SDL_SetNumberProperty(props, SDL_PROP_PROCESS_CREATE_STDOUT_NUMBER, SDL_PROCESS_STDIO_APP);
    (void)SDL_SetNumberProperty(props, SDL_PROP_PROCESS_CREATE_STDIN_NUMBER, SDL_PROCESS_STDIO_NULL);
    (void)SDL_SetNumberProperty(props, SDL_PROP_PROCESS_CREATE_STDERR_NUMBER, SDL_PROCESS_STDIO_APP);
    SDL_Process *p = SDL_CreateProcessWithProperties(props);
    SDL_DestroyProperties(props);

    return p;
}

bool DesktopInterface::ExecuteProcessNonBlocking(const std::string &executable,
                                                 const std::vector<std::string> &arguments)
{
    SDL_Process *p = StartSDLProcess(executable, arguments);
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

std::string DesktopInterface::GetFileArgument(const int argc, char **argv, const std::vector<std::string> &extensions)
{
    for (int i = 0; i < argc; i++)
    {
        const std::string argument = std::string(argv[i]);
        for (const std::string &extension: extensions)
        {
            if (argument.ends_with(extension) && std::filesystem::exists(argument))
            {
                return argument;
            }
        }
    }
    return "";
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
    std::vector<SDL_Process *>::iterator iter = Get().processes.begin();
    while (iter != Get().processes.end())
    {
        SDL_Process *p = *iter.base();
        if (SDL_WaitProcess(p, false, nullptr))
        {
            SDL_DestroyProcess(p);
            iter = Get().processes.erase(iter);
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
