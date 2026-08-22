#include <memory>
#include <string>
#include "game_sdk/WindowManager.h"
#include "MdleditWindow.h"
#include "ModelEditor.h"

int main(const int argc, char **argv)
{
    WindowManager &mgr = WindowManager::Get();
    if (!mgr.Init("GAME SDK Model Editor"))
    {
        return 1;
    }
    mgr.AddWindow(std::make_shared<MdleditWindow>());
    return mgr.Run();
}
