//
// Created by droc101 on 1/20/26.
//

#include <game_sdk/WindowManager.h>
#include <string>
#include "KvleditWindow.h"

int main()
{
    return WindowManager::Run<KvleditWindow>("GAME SDK Key-Value List Editor");
}
