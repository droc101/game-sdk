//
// Created by droc101 on 11/10/25.
//

#include <game_sdk/WindowManager.h>
#include <memory>
#include <string>
#include "LauncherWindow.h"

int main()
{
    WindowManager &mgr = WindowManager::Get();
    if (!mgr.Init("GAME SDK"))
    {
        return 1;
    }
    mgr.AddWindow(std::make_shared<LauncherWindow>());
    return mgr.Run();
}
