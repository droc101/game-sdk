//
// Created by droc101 on 8/31/25.
//

#pragma once

#include <array>
#include <SDL3/SDL_dialog.h>

class DialogFilters
{
    public:
        DialogFilters() = delete;

        // fonedit
        static constexpr std::array<SDL_DialogFileFilter, 1> gfonFilters = {
            SDL_DialogFileFilter{"GAME font (*.gfon)", "gfon"},
        };

        // lvledit
        static constexpr std::array<SDL_DialogFileFilter, 1> gmapFilters = {
            SDL_DialogFileFilter{"Compiled GAME map (*.gmap)", "gmap"},
        };
        static constexpr std::array<SDL_DialogFileFilter, 1> mapJsonFilters = {
            SDL_DialogFileFilter{"GAME map source (*.json)", "json"},
        };

        // mtledit
        static constexpr std::array<SDL_DialogFileFilter, 1> gmtlFilters = {
            SDL_DialogFileFilter{"GAME Material (*.gmtl)", "gmtl"},
        };

        // mdledit
        static constexpr std::array<SDL_DialogFileFilter, 5> modelFilters = {
            SDL_DialogFileFilter{"3D Models (obj, fbx, gltf, dae)", "obj;fbx;gltf;dae"},
            SDL_DialogFileFilter{"Wavefront OBJ Models", "obj"},
            SDL_DialogFileFilter{"FBX Models", "fbx"},
            SDL_DialogFileFilter{"glTF/glTF2.0 Models", "gltf"},
            SDL_DialogFileFilter{"Collada Models", "dae"},
        };
        static constexpr std::array<SDL_DialogFileFilter, 1> objFilters = {
            SDL_DialogFileFilter{"Wavefront OBJ Models", "obj"},
        };
        static constexpr std::array<SDL_DialogFileFilter, 1> gmdlFilters = {
            SDL_DialogFileFilter{"GAME model (*.gmdl)", "gmdl"},
        };

        // shdedit
        static constexpr std::array<SDL_DialogFileFilter, 5> glslFilters = {
            SDL_DialogFileFilter{"GLSL source files (*.glsl, *.vert, *.frag)", "glsl;vert;frag;comp"},
            SDL_DialogFileFilter{"GLSL source (*.glsl)", "glsl"},
            SDL_DialogFileFilter{"GLSL fragment (*.frag)", "frag"},
            SDL_DialogFileFilter{"GLSL vertex (*.vert)", "vert"},
            SDL_DialogFileFilter{"GLSL compute (*.comp)", "comp"},
        };
        static constexpr std::array<SDL_DialogFileFilter, 1> gshdFilters = {
            SDL_DialogFileFilter{"GAME shader (*.gshd)", "gshd"},
        };

        // sndedit
        static constexpr std::array<SDL_DialogFileFilter, 1> gsndFilters = {
            SDL_DialogFileFilter{"GAME sound (*.gsnd)", "gsnd"},
        };
        static constexpr std::array<SDL_DialogFileFilter, 1> wavFilters = {SDL_DialogFileFilter{"WAV audio", "wav"}};

        // texedit
        static constexpr std::array<SDL_DialogFileFilter, 1> gtexFilters = {
            SDL_DialogFileFilter{"GAME texture (*.gtex)", "gtex"},
        };
        static constexpr std::array<SDL_DialogFileFilter, 1> pngFilters = {SDL_DialogFileFilter{"PNG Images", "png"}};
        static constexpr std::array<SDL_DialogFileFilter, 4> imageFilters = {
            SDL_DialogFileFilter{"Images", "png;jpg;jpeg;tga"},
            SDL_DialogFileFilter{"PNG Images", "png"},
            SDL_DialogFileFilter{"JPG Images", "jpg;jepg"},
            SDL_DialogFileFilter{"TGA Images", "tga"},
        };

        // cfgedit
        static constexpr std::array<SDL_DialogFileFilter, 1> gameFilters = {
            SDL_DialogFileFilter{"GAME configuration (*.game)", "game"},
        };

        // kvledit
        static constexpr std::array<SDL_DialogFileFilter, 1> gkvlFilters = {
            SDL_DialogFileFilter{"KvList Asset (*.gkvl)", "gkvl"},
        };
        static constexpr std::array<SDL_DialogFileFilter, 1> kvlJsonFilters = {
            SDL_DialogFileFilter{"KvList source (*.json)", "json"},
        };
};
