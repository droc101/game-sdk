#include <game_sdk/WindowManager.h>
#include <memory>
#include <string>
#include "MapeditWindow.h"

int main()
{
    WindowManager &mgr = WindowManager::Get();
    if (!mgr.Init("GAME SDK Map Editor"))
    {
        return 1;
    }
    mgr.AddWindow(std::make_shared<MapeditWindow>());
    return mgr.Run();
}
