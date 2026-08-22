//
// Created by droc101 on 11/16/25.
//

#include <game_sdk/WindowManager.h>
#include <memory>
#include <string>
#include "MtleditWindow.h"

int main()
{
    WindowManager &mgr = WindowManager::Get();
    if (!mgr.Init("GAME SDK Material Editor"))
    {
        return 1;
    }
    mgr.AddWindow(std::make_shared<MtleditWindow>());
    return mgr.Run();
}
