//
// Created by droc101 on 7/23/25.
//

#include <game_sdk/WindowManager.h>
#include <string>
#include "ShdeditWindow.h"

int main(const int argc, const char **argv)
{
    return WindowManager::Run<ShdeditWindow>(argc, argv, "GAME SDK Shader Editor", false);
}
