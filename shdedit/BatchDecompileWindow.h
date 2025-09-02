//
// Created by droc101 on 8/9/25.
//

#pragma once

#include <string>
#include <vector>
#include <SDL3/SDL_video.h>
#include <libassets/util/Error.h>

class BatchDecompileWindow
{
    public:
        BatchDecompileWindow() = delete;

        static void Show();

        static void Render(SDL_Window *window);

    private:
        inline static bool visible = false;
        inline static std::vector<std::string> files;
        inline static std::string outputFolder;

        static void selectCallback(void * /*userdata*/, const char *const *fileList, int /*filter*/);
        static void outPathCallback(void * /*userdata*/, const char *const *filelist, int /*filter*/);
        static Error::ErrorCode Execute();
};
