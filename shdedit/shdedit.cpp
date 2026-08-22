//
// Created by droc101 on 7/23/25.
//

#include <string>
#include <game_sdk/WindowManager.h>
#include "ShdeditWindow.h"

int main(const int argc, char **argv)
{
    WindowManager &mgr = WindowManager::Get();
    if (!mgr.Init("GAME SDK Shader Editor"))
    {
        return 1;
    }
    mgr.AddWindow(std::make_shared<ShdeditWindow>());
    return mgr.Run();
}
