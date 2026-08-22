#include <game_sdk/WindowManager.h>
#include <memory>
#include <string>
#include "TexeditWindow.h"

int main()
{
    WindowManager &mgr = WindowManager::Get();
    if (!mgr.Init("GAME SDK Texture Editor"))
    {
        return 1;
    }
    mgr.AddWindow(std::make_shared<TexeditWindow>());
    return mgr.Run();
}
