//
// Created by droc101 on 10/20/26.
//

#pragma once

#ifdef ENABLE_RENDERDOC
#include <renderdoc_app.h>
#endif

class RenderdocManager
{
    public:
        RenderdocManager() = delete;

        static bool InitRenderdoc();

        static void DestroyRenderdoc();

        static void StartCapture(const char *captureName);

        static void EndCapture();

    private:
#ifdef ENABLE_RENDERDOC
        static inline void *renderdocLibrary = nullptr;
        static inline RENDERDOC_API_1_7_0 *renderdocApi = nullptr;
#endif
};
