#include <game_sdk/WindowManager.h>
#include <string>
#include "TexeditWindow.h"

int main()
{
    return WindowManager::Run<TexeditWindow>("GAME SDK Texture Editor");
}
