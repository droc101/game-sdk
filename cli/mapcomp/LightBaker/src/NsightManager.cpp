//
// Created by NBT22 on 11/16/26.
//

#include "NsightManager.h"

#ifdef ENABLE_NSIGHT

#ifdef _WIN32
#ifndef NOMINMAX
// Nsight includes windows.h, so tell it not to define min and max
#define NOMINMAX
#endif
#include <array>
#include <cstdint>
#endif

#include <filesystem>
#include <libassets/util/Logger.h>
#include <NGFX_GraphicsCapture_Common.h>
#include <NGFX_GraphicsCapture_Common_Types.h>
#include <NGFX_GraphicsCapture_Vulkan.h>
#include <NGFX_GraphicsCapture_Vulkan_Types.h>
#include <NGFX_Types.h>
#include <string>
#include <vector>

namespace
{
constexpr const char *ResultString(const NGFX_Result result)
{
    switch (result)
    {
        case NGFX_Result_Success:
            return "Success";
        case NGFX_Result_NotImplemented:
            return "Not Implemented";
        case NGFX_Result_LibNotFound:
            return "Library Not Found";
        case NGFX_Result_InvalidLib:
            return "Invalid Library";
        case NGFX_Result_DifferentActivityInjected:
            return "Different Activity Injected";
        case NGFX_Result_InvalidParameter:
            return "Invalid Parameter";
        case NGFX_Result_InvalidState:
            return "Invalid State";
        case NGFX_Result_UnspecifiedError:
            return "Unspecified Error";
        case NGFX_Result_Timeout:
            return "Timeout";
        case NGFX_Result_InsufficientBuffer:
            return "Insufficient Buffer";
        default:
            return "Unknown";
    }
}

/// NGFX_PathChar is wchar_t on Windows and char elsewhere, so go through std::filesystem::path for logging
std::string PathToString(const NGFX_PathChar *path)
{
    return std::filesystem::path(path).string();
}

#ifdef _WIN32
bool InjectGraphicsCapture()
{
    std::array<NGFX_InstallationInfo, 16> installations{};
    uint32_t installationCount = 0;
    const NGFX_Result enumerateResult = NGFX_EnumerateInstallations(installations.data(),
                                                                    installations.size(),
                                                                    &installationCount);
    if (enumerateResult != NGFX_Result_Success || installationCount == 0)
    {
        NGFX_FreeInstallations(installations.data(), installationCount);
        Logger::Error("Failed to find an Nsight Graphics installation: {}", ResultString(enumerateResult));
        return false;
    }

    const NGFX_InstallationInfo &installation = installations.at(0);
    Logger::Info("Injecting Nsight Graphics {}.{}.{} from {}",
                 installation.versionMajor,
                 installation.versionMinor,
                 installation.versionPatch,
                 PathToString(installation.installationPath));

    NGFX_GraphicsCapture_InjectionSettings settings{};
    (void)NGFX_GraphicsCapture_InjectionSettings_SetDefaults(&settings);
    NGFX_GraphicsCapture_Inject_Vulkan_Params injectParams = {
        .version = NGFX_GraphicsCapture_Inject_Vulkan_Params_VER,
        .installationPath = installation.installationPath,
        .settings = &settings,
    };
    const NGFX_Result injectResult = NGFX_GraphicsCapture_Inject_Vulkan(&injectParams);
    NGFX_FreeInstallations(installations.data(), installationCount);
    if (injectResult != NGFX_Result_Success)
    {
        Logger::Error("Failed to inject Nsight Graphics: {}", ResultString(injectResult));
        return false;
    }
    return true;
}
#endif
} // namespace
#endif

bool NsightManager::InitNsight()
{
#ifdef ENABLE_NSIGHT
    NGFX_SetLibraryLoadFn(NGFX_LoadLib_NoVerification); // NOLINT(*-include-cleaner)

#ifdef _WIN32
    if (!InjectGraphicsCapture())
    {
        return false;
    }
#endif

    NGFX_GraphicsCapture_InitializeActivity_Vulkan_Params initializeParams = {
        .version = NGFX_GraphicsCapture_InitializeActivity_Vulkan_Params_VER,
    };
    const NGFX_Result result = NGFX_GraphicsCapture_InitializeActivity_Vulkan(&initializeParams);
    if (result != NGFX_Result_Success)
    {
        Logger::Error("Failed to initialize the Nsight Graphics capture activity: {}", ResultString(result));
        return false;
    }
    Logger::Info("Nsight Graphics SDK initialized");
#endif
    return true;
}

void NsightManager::StartCapture()
{
#ifdef ENABLE_NSIGHT
    NGFX_ArtifactFileCount_Params countParams = {
        .version = NGFX_ArtifactFileCount_Params_VER,
    };
    const NGFX_Result countResult = NGFX_GraphicsCapture_GetCaptureFileCount(&countParams);
    if (countResult != NGFX_Result_Success)
    {
        Logger::Error("Failed to get Nsight Graphics capture file count: {}", ResultString(countResult));
        return;
    }
    captureIndex = countParams.count;

    NGFX_GraphicsCapture_RequestCapture_Vulkan_Params requestParams = {
        .version = NGFX_GraphicsCapture_RequestCapture_Vulkan_Params_VER,
        .delimiter = NGFX_GraphicsCapture_Delimiter_VkFrameBoundaryEXT,
        .framesBeforeStart = 0,
        .framesToCapture = 1,
    };
    const NGFX_Result requestResult = NGFX_GraphicsCapture_RequestCapture_Vulkan(&requestParams);
    if (requestResult != NGFX_Result_Success)
    {
        Logger::Error("Failed to request Nsight Graphics capture: {}", ResultString(requestResult));
        return;
    }
    captureRequested = true;
#endif
}

void NsightManager::EndCapture()
{
#ifdef ENABLE_NSIGHT
    if (!captureRequested)
    {
        return;
    }
    captureRequested = false;

    Logger::Info("Waiting for Nsight Graphics capture to be written...");
    NGFX_WaitForArtifactFilePath_Params waitParams = {
        .version = NGFX_WaitForArtifactFilePath_Params_VER,
        .artifactIndex = captureIndex,
        .timeoutMs = -1,
    };
    NGFX_Result result = NGFX_GraphicsCapture_WaitForCaptureFilePath(&waitParams);
    if ((result == NGFX_Result_Success || result == NGFX_Result_InsufficientBuffer) &&
        waitParams.requiredPathCapacity > 0)
    {
        std::vector<NGFX_PathChar> path(waitParams.requiredPathCapacity);
        waitParams.filePath = path.data();
        waitParams.filePathCapacity = path.size();
        result = NGFX_GraphicsCapture_WaitForCaptureFilePath(&waitParams);
        if (result == NGFX_Result_Success)
        {
            Logger::Info("Nsight Graphics capture saved to {}", PathToString(path.data()));
            return;
        }
    }
    Logger::Error("Failed to make Nsight Graphics capture: {}", ResultString(result));
#endif
}
