#include <game_sdk/WindowManager.h>
#include <string>
#include "MdleditWindow.h"

int main(const int argc, const char **argv)
{
    return WindowManager::Run<MdleditWindow>(argc, argv, "GAME SDK Model Editor");
}
