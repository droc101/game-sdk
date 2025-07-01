//
// Created by droc101 on 6/23/25.
//

#pragma once

#include <cstdint>
#include <vector>

class DataReader;
class AssetReader
{
    public:
        enum class AssetType : uint32_t // NOLINT(*-enum-size)
        {
            ASSET_TYPE_TEXTURE = 0,
            ASSET_TYPE_MP3 = 1,
            ASSET_TYPE_WAV = 2,
            ASSET_TYPE_LEVEL = 3,
            ASSET_TYPE_GLSL = 4,
            ASSET_TYPE_SPIRV_FRAG = 5,
            ASSET_TYPE_SPIRV_VERT = 6,
            ASSET_TYPE_MODEL = 7,
            ASSET_TYPE_FONT = 8
        };

        /**
         * Do not create this. Don't do it. Do not.
         */
        AssetReader() = delete;

        /**
         * Get the uncompressed payload data of a texture
         * @param asset The complete asset file
         * @param[in, out] reader The DataReader to use
         * @return The asset type
         */
        [[nodiscard]] static AssetType Decompress(std::vector<uint8_t> &asset, DataReader &reader);

        /**
         * Create an asset given the uncompressed payload
         * @param inBuffer The payload data to compress
         * @param[out] outBuffer The buffer to output the compressed data into
         * @param type The asset type to store
         */
        static void Compress(std::vector<uint8_t> &inBuffer, std::vector<uint8_t> &outBuffer, AssetType type);

        /**
         * Get the data of an asset from a file
         * @param filePath The path to the file
         * @param[in, out] reader The DataReader to use
         * @return The asset type
         */
        [[nodiscard]] static AssetType LoadFromFile(const char *filePath, DataReader &reader);

        /**
         * Create an asset file on disk
         * @param filePath The file to save as
         * @param data The payload data
         * @param type The type of asset
         */
        static void SaveToFile(const char *filePath, std::vector<uint8_t> &data, AssetType type);
};
