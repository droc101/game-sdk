//
// Created by droc101 on 8/31/25.
//

#pragma once

#include <SDL3/SDL_dialog.h>
#include <vector>

class DialogFilters
{
    public:
        DialogFilters() = delete;

        static inline const std::vector<SDL_DialogFileFilter> EXE_FILTERS = {
#ifdef SDL_PLATFORM_LINUX
            SDL_DialogFileFilter{.name = "Program (*)", .pattern = "*"}
#else
            SDL_DialogFileFilter{"Program (*.exe)", "exe"}
#endif
        };

        static inline const std::vector<SDL_DialogFileFilter> LOG_FILTERS = {
            SDL_DialogFileFilter{.name = "Log File (*.log)", .pattern = "log"},
        };

        // fonedit
        static inline const std::vector<SDL_DialogFileFilter> GFON_FILTERS = {
            SDL_DialogFileFilter{.name = "GAME font (*.gfon)", .pattern = "gfon"},
        };

        // lvledit
        static inline const std::vector<SDL_DialogFileFilter> GMAP_FILTERS = {
            SDL_DialogFileFilter{.name = "Compiled GAME map (*.gmap)", .pattern = "gmap"},
        };
        static inline const std::vector<SDL_DialogFileFilter> MAP_JSON_FILTERS = {
            SDL_DialogFileFilter{.name = "GAME map source (*.json)", .pattern = "json"},
        };

        // mtledit
        static inline const std::vector<SDL_DialogFileFilter> GMTL_FILTERS = {
            SDL_DialogFileFilter{.name = "GAME Material (*.gmtl)", .pattern = "gmtl"},
        };

        // mdledit
        static inline const std::vector<SDL_DialogFileFilter> STANDARD_MODEL_FILTERS = {
            SDL_DialogFileFilter{.name = "3D Models (obj, fbx, gltf, dae)", .pattern = "obj;fbx;gltf;dae"},
            SDL_DialogFileFilter{.name = "Wavefront OBJ Models", .pattern = "obj"},
            SDL_DialogFileFilter{.name = "FBX Models", .pattern = "fbx"},
            SDL_DialogFileFilter{.name = "glTF/glTF2.0 Models", .pattern = "gltf"},
            SDL_DialogFileFilter{.name = "Collada Models", .pattern = "dae"},
        };
        static inline const std::vector<SDL_DialogFileFilter> OBJ_FILTERS = {
            SDL_DialogFileFilter{.name = "Wavefront OBJ Models", .pattern = "obj"},
        };
        static inline const std::vector<SDL_DialogFileFilter> GMDL_FILTERS = {
            SDL_DialogFileFilter{.name = "GAME model (*.gmdl)", .pattern = "gmdl"},
        };

        // shdedit
        static inline const std::vector<SDL_DialogFileFilter> GLSL_FILTERS = {
            SDL_DialogFileFilter{.name = "GLSL source files (*.glsl, *.vert, *.frag)",
                                 .pattern = "glsl;vert;frag;comp"},
            SDL_DialogFileFilter{.name = "GLSL source (*.glsl)", .pattern = "glsl"},
            SDL_DialogFileFilter{.name = "GLSL fragment (*.frag)", .pattern = "frag"},
            SDL_DialogFileFilter{.name = "GLSL vertex (*.vert)", .pattern = "vert"},
            SDL_DialogFileFilter{.name = "GLSL compute (*.comp)", .pattern = "comp"},
        };
        static inline const std::vector<SDL_DialogFileFilter> GSHD_FILTERS = {
            SDL_DialogFileFilter{.name = "GAME shader (*.gshd)", .pattern = "gshd"},
        };

        // sndedit
        static inline const std::vector<SDL_DialogFileFilter> GSND_FILTERS = {
            SDL_DialogFileFilter{.name = "GAME sound (*.gsnd)", .pattern = "gsnd"},
        };
        static inline const std::vector<SDL_DialogFileFilter> WAV_FILTERS = {
            SDL_DialogFileFilter{.name = "WAV audio", .pattern = "wav"},
        };

        // texedit
        static inline const std::vector<SDL_DialogFileFilter> GTEX_FILTERS = {
            SDL_DialogFileFilter{.name = "GAME texture (*.gtex)", .pattern = "gtex"},
        };
        static inline const std::vector<SDL_DialogFileFilter> PNG_FILTERS = {
            SDL_DialogFileFilter{.name = "PNG Images", .pattern = "png"},
        };
        static inline const std::vector<SDL_DialogFileFilter> EXR_FILTERS = {
            SDL_DialogFileFilter{.name = "EXR Images", .pattern = "exr"},
        };
        static inline const std::vector<SDL_DialogFileFilter> IMAGE_FILTERS = {
            SDL_DialogFileFilter{.name = "Images", .pattern = "png;exr"},
            SDL_DialogFileFilter{.name = "PNG Images", .pattern = "png"},
            SDL_DialogFileFilter{.name = "EXR Images", .pattern = "exr"},
        };

        // kvledit
        static inline const std::vector<SDL_DialogFileFilter> GKVL_FILTERS = {
            SDL_DialogFileFilter{.name = "KvList Asset (*.gkvl)", .pattern = "gkvl"},
        };
        static inline const std::vector<SDL_DialogFileFilter> KVL_JSON_FILTERS = {
            SDL_DialogFileFilter{.name = "KvList source (*.json)", .pattern = "json"},
        };
};
