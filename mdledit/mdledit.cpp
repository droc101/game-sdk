#include <format>
#include <game_sdk/DesktopInterface.h>
#include <game_sdk/DialogFilters.h>
#include <game_sdk/Options.h>
#include <game_sdk/SDKWindow.h>
#include <game_sdk/SharedMgr.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <libassets/asset/ModelAsset.h>
#include <libassets/util/Error.h>
#include <libassets/util/Logger.h>
#include <string>
#include <utility>

#include "game_sdk/WindowManager.h"
#include "MdleditWindow.h"
#include "ModelEditor.h"
#include "tabs/CollisionTab.h"
#include "tabs/LodsTab.h"
#include "tabs/MaterialsTab.h"
#include "tabs/PreviewOptionsTab.h"
#include "tabs/SkinsTab.h"

int main(const int argc, char **argv)
{
    WindowManager &mgr = WindowManager::Get();
    if (!mgr.Init("GAME SDK Model Editor"))
    {
        return 1;
    }
    mgr.AddWindow(std::make_shared<MdleditWindow>());
    return mgr.Run();
}
