//
// Created by droc101 on 8/9/25.
//

#pragma once
#include <libassets/asset/ShaderAsset.h>
#include <SDL3/SDL_video.h>
#include <string>
#include <vector>


class BatchCompileWindow
{
    public:
        BatchCompileWindow() = delete;

        static void Show();

        static void Render(SDL_Window *window);

    private:
        static inline bool visible = false;
        static inline std::vector<std::string> files;
        static inline std::vector<ShaderAsset::ShaderType> types;
        static inline bool targetOpenGL = false;
        static inline std::string outputFolder;

        static void selectCallback(void * /*userdata*/, const char *const *fileList, int /*filter*/);
        static void outPathCallback(void * /*userdata*/, const char *const *filelist, int /*filter*/);
        static Error::ErrorCode Execute();
};
