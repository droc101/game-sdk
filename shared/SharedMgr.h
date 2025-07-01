//
// Created by droc101 on 7/1/25.
//

#ifndef SHAREDUIMGR_H
#define SHAREDUIMGR_H
#include <SDL3/SDL_video.h>

class SharedMgr {
    public:
        SharedMgr() = delete;

        static void InitSharedMgr();

        static void SharedMenuUI();

        static void RenderSharedUI(SDL_Window* window);

        static void DestroySharedMgr();

    private:
        inline static bool metricsVisible = false;
        inline static bool demoVisible = false;
};



#endif //SHAREDUIMGR_H
