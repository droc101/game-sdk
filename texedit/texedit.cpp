#include <game_sdk/WindowManager.h>
#include <string>
#include "TexeditWindow.h"

int main(const int argc, const char **argv)
{
    return WindowManager::Run<TexeditWindow>(argc, argv, "GAME SDK Texture Editor", false);
}
