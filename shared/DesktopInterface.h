//
// Created by droc101 on 11/17/25.
//

#pragma once

#include <string>
#include <vector>

class DesktopInterface
{
    public:
        DesktopInterface() = delete;

        static bool ExecuteProcess(const std::string& executable,
                                   const std::vector<std::string> &arguments, int *exitCode);

        static bool ExecuteProcessNonBlocking(const std::string& executable,
                                   const std::vector<std::string> &arguments);

        static bool OpenFilesystemPath(const std::string& path);

        static bool OpenURL(const std::string &url);
};
