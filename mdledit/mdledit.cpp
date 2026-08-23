#include <game_sdk/WindowManager.h>
#include <string>
#include "MdleditWindow.h"

int main()
{
    return WindowManager::Run<MdleditWindow>("GAME SDK Model Editor");
}
