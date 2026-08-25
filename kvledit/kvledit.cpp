//
// Created by droc101 on 1/20/26.
//

#include <game_sdk/WindowManager.h>
#include <string>
#include "KvleditWindow.h"

int main(const int argc, const char **argv)
{
    return WindowManager::Run<KvleditWindow>(argc, argv, "GAME SDK Key-Value List Editor", false);
}
