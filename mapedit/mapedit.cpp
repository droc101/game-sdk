#include <memory>
#include <string>
#include "game_sdk/WindowManager.h"
#include "MapeditWindow.h"

int main(const int argc, char **argv)
{
    WindowManager &mgr = WindowManager::Get();
    if (!mgr.Init("GAME SDK Map Editor"))
    {
        return 1;
    }
    mgr.AddWindow(std::make_shared<MapeditWindow>());
    return mgr.Run();
}
