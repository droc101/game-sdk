//
// Created by droc101 on 11/10/25.
//

#include <game_sdk/WindowManager.h>
#include <string>
#include "LauncherWindow.h"

int main(const int argc, const char **argv)
{
    return WindowManager::Run<LauncherWindow>(argc, argv, "GAME SDK", false);
}
