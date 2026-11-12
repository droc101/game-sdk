//
// Created by droc101 on 10/20/26.
//

#include "RenderdocManager.h"

#ifdef ENABLE_RENDERDOC
#include <cerrno>
#include <cstring>
#include <dlfcn.h>
#include <libassets/util/Logger.h>
#include <renderdoc_app.h>
#endif

bool RenderdocManager::InitRenderdoc()
{
#ifdef ENABLE_RENDERDOC
    renderdocLibrary = dlopen("librenderdoc.so", RTLD_NOW);
    if (renderdocLibrary != nullptr)
    {
        pRENDERDOC_GetAPI RENDERDOC_GetAPI = reinterpret_cast<pRENDERDOC_GetAPI>(dlsym(renderdocLibrary,
                                                                                       "RENDERDOC_GetAPI"));
        const int ret = RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_7_0, reinterpret_cast<void **>(&renderdocApi));
        if (ret != 1)
        {
            Logger::Error("RENDERDOC_GetAPI failed!");
            return false;
        }
        Logger::Info("RenderDoc API initialized");
        return true;
    }
    Logger::Error("failed to dlopen librenderdoc.so: {}", strerror(errno));
#endif
    return true;
}

void RenderdocManager::DestroyRenderdoc()
{
#ifdef ENABLE_RENDERDOC
    if (renderdocApi != nullptr)
    {
        renderdocApi->Shutdown();
        renderdocApi = nullptr;
    }
    if (renderdocLibrary != nullptr)
    {
        dlclose(renderdocLibrary);
        renderdocLibrary = nullptr;
    }
#endif
}

void RenderdocManager::StartCapture(const char *captureName)
{
#ifdef ENABLE_RENDERDOC
    if (renderdocApi != nullptr)
    {
        renderdocApi->TriggerCapture();
        renderdocApi->StartFrameCapture(nullptr, nullptr);
        renderdocApi->SetCaptureTitle(captureName);
    }
#endif
}

void RenderdocManager::EndCapture()
{
#ifdef ENABLE_RENDERDOC
    if (renderdocApi != nullptr)
    {
        if (renderdocApi->EndFrameCapture(nullptr, nullptr) != 1)
        {
            Logger::Error("Failed to make RenderDoc capture!");
        }
    }
#endif
}
