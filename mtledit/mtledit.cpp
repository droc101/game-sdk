//
// Created by droc101 on 11/16/25.
//

#include <game_sdk/WindowManager.h>
#include <string>
#include "MtleditWindow.h"

int main(const int argc, const char **argv)
{
    return WindowManager::Run<MtleditWindow>(argc, argv, "GAME SDK Material Editor");
}
