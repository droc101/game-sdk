#include <game_sdk/WindowManager.h>
#include <string>
#include "SndeditWindow.h"

int main()
{
    return WindowManager::Run<SndeditWindow>("GAME SDK Sound Editor");
}
