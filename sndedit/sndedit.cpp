#include <game_sdk/WindowManager.h>
#include <string>
#include "SndeditWindow.h"

int main(const int argc, const char **argv)
{
    return WindowManager::Run<SndeditWindow>(argc, argv, "GAME SDK Sound Editor", false);
}
