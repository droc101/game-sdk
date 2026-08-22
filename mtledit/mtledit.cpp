//
// Created by droc101 on 11/16/25.
//

#include <filesystem>
#include <string>

#include "game_sdk/WindowManager.h"
#include "MtleditWindow.h"

int main(const int argc, const char **argv)
{
    WindowManager &mgr = WindowManager::Get();
    if (!mgr.Init("GAME SDK Material Editor"))
    {
        return 1;
    }
    mgr.AddWindow(std::make_shared<MtleditWindow>());
    return mgr.Run();
}
