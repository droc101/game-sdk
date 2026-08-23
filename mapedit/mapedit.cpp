#include <game_sdk/WindowManager.h>
#include <string>
#include "MapeditWindow.h"

int main()
{
    return WindowManager::Run<MapeditWindow>("GAME SDK Map Editor");
}
