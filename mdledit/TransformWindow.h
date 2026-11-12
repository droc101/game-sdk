//
// Created by droc101 on 9/14/26.
//

#ifndef GAME_SDK_TRANSFORMWINDOW_H
#define GAME_SDK_TRANSFORMWINDOW_H

#include <game_sdk/Window.h>
#include <glm/vec3.hpp>

class TransformWindow final: public Window
{
    public:
        TransformWindow() = default;

    protected:
        void Render() override;
        [[nodiscard]] const WindowProperties &GetProperties() const override;

    private:
        WindowProperties properties = {
            .title = "Scale Model",
            .defaultSize = glm::ivec2(400, 120),
            .icon = "",
            .defaultFlags = 0,
            .defaultImguiWindow = true,
        };

        glm::vec3 scale = glm::vec3(1,1,1);

        bool applyToLods = true;
        bool applyToCollision = true;
        bool updateBoundingBox = true;
};


#endif //GAME_SDK_TRANSFORMWINDOW_H
