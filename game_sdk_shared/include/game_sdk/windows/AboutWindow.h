//
// Created by droc101 on 6/29/25.
//

#pragma once

#include <map>
#include <string>

class AboutWindow
{
    public:
        static AboutWindow &Get();

        void Show();

        void Hide();

        void Render();

    private:
        AboutWindow() = default;

        bool visible = false;

        std::string selectedComponent{};
        std::map<std::string, std::string> thirdPartyComponents{};
};
