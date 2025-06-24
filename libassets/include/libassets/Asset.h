//
// Created by droc101 on 6/23/25.
//

#ifndef ASSET_H
#define ASSET_H
#include <cstdint>

/**
 * Base class for GAME assets
 */
class Asset {
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

        virtual ~Asset() = default;
        /**
         * Do not use this. Don't do it. Do not.
         */
        explicit Asset();
        /**
         * Create an asset from a file on disk
         * @param path
         */
        explicit Asset(const char *path);
        /**
         * Create an asset from a byte buffer
         * @param data The buffer to create the asset from
         */
        explicit Asset(uint8_t *data);

        /**
         * Save the data of this asset to a buffer
         */
        [[nodiscard]] virtual uint8_t* SaveToBuffer();

        /**
         * Save this asset to a file
         * @param path The file path to save to
         */
        void SaveToFile(const char *path);

        /**
         * Get the asset type
         */
        [[nodiscard]] AssetType GetAssetType() const;

        /**
         * Set the asset type
         * @param assetType The new asset type
         */
        void SetAssetType(AssetType assetType);

        /**
         * Finish loading this asset
         */
        virtual void FinishLoading();

    protected:
        AssetType asset_type;
        uint8_t *tempCompressedData;

        /**
         * Decompress asset data
         * @param asset The header compressed data
         * @param outSize Where to store the size of the decompressed buffer, or nullptr to discard it (not recommended)
         * @return The decompressed data without a header
         */
        [[nodiscard]] uint8_t *Decompress(uint8_t *asset, std::size_t *outSize = nullptr);

        /**
         * Compress asset data and add a header
         * @param data The data to compress
         * @param data_size The size of the data to compress
         * @return The compressed data with a header
         */
        [[nodiscard]] const uint8_t *Compress(uint8_t *data, std::size_t data_size) const ;
};



#endif //ASSET_H
