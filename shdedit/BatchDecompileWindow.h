//
// Created by droc101 on 8/9/25.
//

#pragma once

#include <libassets/util/Error.h>
#include <string>
#include <vector>

class BatchDecompileWindow
{
    public:
        BatchDecompileWindow() = delete;

        static void Show();

        static void Render();

    private:
        static inline bool visible = false;
        static inline std::vector<std::string> files;
        static inline std::string outputFolder;

        static void selectCallback(const std::vector<std::string> &paths);
        static void outPathCallback(const std::string &path);
        static Error::ErrorCode Execute();
};
