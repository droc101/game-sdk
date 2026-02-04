//
// Created by droc101 on 8/20/25.
//

#pragma once

class CollisionTab
{
    public:
        CollisionTab() = delete;

        static void Render();

    private:
        static void RenderCHullUI();

        static void RenderStaticMeshUI();
};
