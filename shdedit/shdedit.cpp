//
// Created by droc101 on 7/23/25.
//

#include <game_sdk/WindowManager.h>
#include <memory>
#include <string>
#include "ShdeditWindow.h"

int main()
{
    WindowManager &mgr = WindowManager::Get();
    if (!mgr.Init("GAME SDK Shader Editor"))
    {
        return 1;
    }
    mgr.AddWindow(std::make_shared<ShdeditWindow>());
    return mgr.Run();
}
