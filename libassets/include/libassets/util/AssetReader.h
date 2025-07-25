//
// Created by droc101 on 6/23/25.
//

#pragma once

#include <cstdint>
#include <vector>
#include <libassets/util/Asset.h>
#include <libassets/util/Error.h>

class AssetReader
{
    public:
        /**
         * Do not create this. Don't do it. Do not.
         */
        AssetReader() = delete;

        [[nodiscard]] static Error::ErrorCode Decompress(std::vector<uint8_t> &asset, Asset &outAsset);

        /**
         * Create an asset given the uncompressed payload
         * @param inBuffer The payload data to compress
         * @param[out] outBuffer The buffer to output the compressed data into
         * @param type The asset type to store
         * @param typeVersion
         */
        [[nodiscard]] static Error::ErrorCode Compress(std::vector<uint8_t> &inBuffer,
                                                       std::vector<uint8_t> &outBuffer,
                                                       Asset::AssetType type,
                                                       uint8_t typeVersion);

        [[nodiscard]] static Error::ErrorCode LoadFromFile(const char *filePath, Asset &outAsset);

        /**
         * Create an asset file on disk
         * @param filePath The file to save as
         * @param data The payload data
         * @param type The type of asset
         * @param typeVersion
         */
        [[nodiscard]] static Error::ErrorCode SaveToFile(const char *filePath,
                                                         std::vector<uint8_t> &data,
                                                         Asset::AssetType type,
                                                         uint8_t typeVersion);
};
