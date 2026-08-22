#include <game_sdk/WindowManager.h>
#include <memory>
#include <string>
#include "SndeditWindow.h"

int main()
{
    WindowManager &mgr = WindowManager::Get();
    if (!mgr.Init("GAME SDK Sound Editor"))
    {
        return 1;
    }
    mgr.AddWindow(std::make_shared<SndeditWindow>());
    return mgr.Run();
}
