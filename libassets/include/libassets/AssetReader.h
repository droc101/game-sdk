//
// Created by droc101 on 6/23/25.
//

#ifndef ASSET_H
#define ASSET_H
#include <cstdint>

class AssetReader
{
    public:
        enum AssetType
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
         * @param outSize Where to store the payload size
         * @param outAssetType Where to store the payload type
         * @return The payload data
         */
        [[nodiscard]] static uint8_t *Decompress(uint8_t *asset, std::size_t *outSize, AssetType *outAssetType);

        /**
         * Create an asset given the uncompressed payload
         * @param data The payload data
         * @param data_size The size of the payload data
         * @param out_compressed_size Where to store the size of the resulting asset
         * @param type The asset type to store
         * @return The asset data
         */
        [[nodiscard]] static const uint8_t *Compress(uint8_t *data,
                                                     std::size_t data_size,
                                                     std::size_t *out_compressed_size,
                                                     AssetType type);

        /**
         * Get the data of an asset from a file
         * @param filePath The path to the file
         * @param outSize Where to store the size of the payload
         * @param outType Where to store the type of the asset
         * @return The payload data
         */
        [[nodiscard]] static uint8_t *LoadFromFile(const char *filePath, std::size_t *outSize, AssetType *outType);

        /**
         * Create an asset file on disk
         * @param filePath The file to save as
         * @param data The payload data
         * @param dataSize The size of the payload data
         * @param type The type of asset
         */
        static void SaveToFile(const char *filePath, uint8_t *data, std::size_t dataSize, AssetType type);
};


#endif //ASSET_H
