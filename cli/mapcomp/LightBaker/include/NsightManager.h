//
// Created by NBT22 on 11/16/26.
//

#pragma once

#ifdef ENABLE_NSIGHT
#include <cstdint>
#endif

class NsightManager
{
    public:
        NsightManager() = delete;

        static bool InitNsight();

        static void StartCapture();

        static void EndCapture();

    private:
#ifdef ENABLE_NSIGHT
        static inline bool captureRequested = false;
        static inline uint32_t captureIndex = 0;
#endif
};
