//
// Created by droc101 on 7/18/25.
//

#ifndef ASSET_H
#define ASSET_H
#include <libassets/util/DataReader.h>


class Asset
{
    public:
        enum class AssetType : uint32_t // NOLINT(*-enum-size)
        {
            ASSET_TYPE_TEXTURE = 0,
            ASSET_TYPE_MP3_DEPRECIATED_DONT_USE [[deprecated("This has been replaced fully by WAV files.")]] = 1,
            ASSET_TYPE_WAV = 2,
            ASSET_TYPE_LEVEL = 3,
            ASSET_TYPE_GLSL = 4,
            ASSET_TYPE_SPIRV_FRAG = 5,
            ASSET_TYPE_SPIRV_VERT = 6,
            ASSET_TYPE_MODEL = 7,
            ASSET_TYPE_FONT = 8
        };

        Asset() = default;

        AssetType type{};
        size_t size{};
        DataReader reader{};
};


#endif //ASSET_H
