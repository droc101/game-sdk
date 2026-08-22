//
// Created by droc101 on 1/20/26.
//

#include <string>
#include "game_sdk/WindowManager.h"
#include "KvleditWindow.h"

int main(const int argc, char **argv)
{
    WindowManager &mgr = WindowManager::Get();
    if (!mgr.Init("GAME SDK Key-Value List Editor"))
    {
        return 1;
    }
    mgr.AddWindow(std::make_shared<KvleditWindow>());
    return mgr.Run();
}
