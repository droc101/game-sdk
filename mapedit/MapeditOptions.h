//
// Created by droc101 on 10/29/26.
//

#pragma once
#include <string>


class MapeditOptions
{
    public:
        MapeditOptions() = default;

        /**
         * Load (or reload) options from disk
         */
        void Load();

        /**
         * Load default options
         */
        void LoadDefault();

        /**
         * Save options to disk
         */
        void Save();

        std::string defaultMaterial{};

        bool drawGrid = true;
        bool drawAxisHelper = true;
        bool drawWorldBorder = true;
        bool drawViewportInfo = true;
        bool snapToGrid = true;
        bool showSidebar = true;
        bool drawModels = true;
        bool culling = true;
        bool showInternalBuffers = false;
        bool show2dViewports = true;
        bool showInvisibleFaces = true;

    private:
        static inline const char *DEFAULT_MATERIAL = "material/tools/invisible.gmtl";
};
