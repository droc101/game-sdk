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

        static inline const std::vector<SDL_DialogFileFilter> exeFilters = {
#ifdef SDL_PLATFORM_LINUX
            SDL_DialogFileFilter{"Program (*)", "*"}
#else
            SDL_DialogFileFilter{"Program (*.exe)", "exe"}
#endif
        };

        // fonedit
        static inline const std::vector<SDL_DialogFileFilter> gfonFilters = {
            SDL_DialogFileFilter{"GAME font (*.gfon)", "gfon"},
        };

        // lvledit
        static inline const std::vector<SDL_DialogFileFilter> gmapFilters = {
            SDL_DialogFileFilter{"Compiled GAME map (*.gmap)", "gmap"},
        };
        static inline const std::vector<SDL_DialogFileFilter> mapJsonFilters = {
            SDL_DialogFileFilter{"GAME map source (*.json)", "json"},
        };

        // mtledit
        static inline const std::vector<SDL_DialogFileFilter> gmtlFilters = {
            SDL_DialogFileFilter{"GAME Material (*.gmtl)", "gmtl"},
        };

        // mdledit
        static inline const std::vector<SDL_DialogFileFilter> modelFilters = {
            SDL_DialogFileFilter{"3D Models (obj, fbx, gltf, dae)", "obj;fbx;gltf;dae"},
            SDL_DialogFileFilter{"Wavefront OBJ Models", "obj"},
            SDL_DialogFileFilter{"FBX Models", "fbx"},
            SDL_DialogFileFilter{"glTF/glTF2.0 Models", "gltf"},
            SDL_DialogFileFilter{"Collada Models", "dae"},
        };
        static inline const std::vector<SDL_DialogFileFilter> objFilters = {
            SDL_DialogFileFilter{"Wavefront OBJ Models", "obj"},
        };
        static inline const std::vector<SDL_DialogFileFilter> gmdlFilters = {
            SDL_DialogFileFilter{"GAME model (*.gmdl)", "gmdl"},
        };

        // shdedit
        static inline const std::vector<SDL_DialogFileFilter> glslFilters = {
            SDL_DialogFileFilter{"GLSL source files (*.glsl, *.vert, *.frag)", "glsl;vert;frag;comp"},
            SDL_DialogFileFilter{"GLSL source (*.glsl)", "glsl"},
            SDL_DialogFileFilter{"GLSL fragment (*.frag)", "frag"},
            SDL_DialogFileFilter{"GLSL vertex (*.vert)", "vert"},
            SDL_DialogFileFilter{"GLSL compute (*.comp)", "comp"},
        };
        static inline const std::vector<SDL_DialogFileFilter> gshdFilters = {
            SDL_DialogFileFilter{"GAME shader (*.gshd)", "gshd"},
        };

        // sndedit
        static inline const std::vector<SDL_DialogFileFilter> gsndFilters = {
            SDL_DialogFileFilter{"GAME sound (*.gsnd)", "gsnd"},
        };
        static inline const std::vector<SDL_DialogFileFilter> wavFilters = {SDL_DialogFileFilter{"WAV audio", "wav"}};

        // texedit
        static inline const std::vector<SDL_DialogFileFilter> gtexFilters = {
            SDL_DialogFileFilter{"GAME texture (*.gtex)", "gtex"},
        };
        static inline const std::vector<SDL_DialogFileFilter> pngFilters = {SDL_DialogFileFilter{"PNG Images", "png"}};
        static inline const std::vector<SDL_DialogFileFilter> imageFilters = {
            SDL_DialogFileFilter{"Images", "png;jpg;jpeg;tga"},
            SDL_DialogFileFilter{"PNG Images", "png"},
            SDL_DialogFileFilter{"JPG Images", "jpg;jepg"},
            SDL_DialogFileFilter{"TGA Images", "tga"},
        };

        // cfgedit
        static inline const std::vector<SDL_DialogFileFilter> gameFilters = {
            SDL_DialogFileFilter{"GAME configuration (*.game)", "game"},
        };

        // kvledit
        static inline const std::vector<SDL_DialogFileFilter> gkvlFilters = {
            SDL_DialogFileFilter{"KvList Asset (*.gkvl)", "gkvl"},
        };
        static inline const std::vector<SDL_DialogFileFilter> kvlJsonFilters = {
            SDL_DialogFileFilter{"KvList source (*.json)", "json"},
        };
};
