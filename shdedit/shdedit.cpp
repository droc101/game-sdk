//
// Created by droc101 on 7/23/25.
//

#include <game_sdk/WindowManager.h>
#include <string>
#include "ShdeditWindow.h"

int main()
{
    return WindowManager::Run<ShdeditWindow>("GAME SDK Shader Editor");
}
