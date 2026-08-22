#include <game_sdk/WindowManager.h>
#include <memory>
#include <string>
#include "MdleditWindow.h"

int main()
{
    WindowManager &mgr = WindowManager::Get();
    if (!mgr.Init("GAME SDK Model Editor"))
    {
        return 1;
    }
    mgr.AddWindow(std::make_shared<MdleditWindow>());
    return mgr.Run();
}
