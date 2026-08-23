//
// Created by droc101 on 11/16/25.
//

#include <game_sdk/WindowManager.h>
#include <string>
#include "MtleditWindow.h"

int main()
{
    return WindowManager::Run<MtleditWindow>("GAME SDK Material Editor");
}
