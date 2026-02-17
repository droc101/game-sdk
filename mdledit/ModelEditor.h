//
// Created by droc101 on 2/17/26.
//

#ifndef GAME_SDK_MODELEDITOR_H
#define GAME_SDK_MODELEDITOR_H

#include <game_sdk/ModelViewer.h>
#include <string>

class ModelEditor
{
    public:
        static inline ModelViewer modelViewer{};
        static inline bool modelLoaded = false;

        static void DestroyExistingModel();

        static void ImportLod(const std::string &path);

        static void ImportSingleHull(const std::string &path);

        static void ImportMultipleHulls(const std::string &path);

        static void ImportStaticCollider(const std::string &path);
};


#endif //GAME_SDK_MODELEDITOR_H
