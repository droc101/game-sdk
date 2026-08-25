#include <game_sdk/WindowManager.h>
#include <string>
#include "MapeditWindow.h"

int main(const int argc, const char **argv)
{
    return WindowManager::Run<MapeditWindow>(argc, argv, "GAME SDK Map Editor");
}
