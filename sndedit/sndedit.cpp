#include <game_sdk/WindowManager.h>
#include <string>
#include "SndeditWindow.h"

int main(const int argc, char **argv)
{
    WindowManager &mgr = WindowManager::Get();
    if (!mgr.Init("GAME SDK Sound Editor"))
    {
        return 1;
    }
    mgr.AddWindow(std::make_shared<SndeditWindow>());
    return mgr.Run();
}
