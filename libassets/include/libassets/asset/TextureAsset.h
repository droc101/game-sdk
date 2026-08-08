//
// Created by droc101 on 6/23/25.
//

#pragma once

#include <cstddef>
#include <cstdint>
#include <libassets/asset/Asset.h>
#include <libassets/util/DataReader.h>
#include <libassets/util/DataWriter.h>
#include <libassets/util/Error.h>
#include <string>
#include <vector>

class TextureAsset final: public Asset
{
    public:
        enum class PixelFormat : uint8_t
        {
            /// uint8_t per channel, 4 bytes total
            RGBA8,
            /// 16-bit float (aka half float) per channel, 8 bytes total
            RGBAF16,
        };

        /**
         * Please use @c TextureAsset::Create* instead.
         */
        TextureAsset();

        bool filter = false;
        bool repeat = true;
        bool mipmaps = true;

        void Reset() override;

        [[nodiscard]] Error::ErrorCode LoadFromBuffer(DataReader &reader) override;
        [[nodiscard]] Error::ErrorCode SaveToBuffer(DataWriter &writer) const override;

        [[nodiscard]] Error::ErrorCode LoadFromAsset(const std::string &filePath) override;

        [[nodiscard]] Error::ErrorCode Import(const std::string &filePath) override;
        [[nodiscard]] Error::ErrorCode Export(const std::string &filePath) const override;

        [[nodiscard]] AssetType GetAssetType() const override;
        [[nodiscard]] uint8_t GetAssetTypeVersion() const override;

        /// Create a TextureAsset with the "missing texture" pattern
        void CreateMissingTexture();

        /// Get the pixel data in RGBA format
        [[nodiscard]] uint8_t *GetPixelsRGBA();
        [[nodiscard]] const uint8_t *GetPixelsRGBA() const;

        /// Get the width of the texture
        [[nodiscard]] uint32_t GetWidth() const;

        /// Get the height of the texture
        [[nodiscard]] uint32_t GetHeight() const;

        /// Get the size of the pixel data in bytes
        [[nodiscard]] size_t GetPixelDataSize() const;

        /**
         * Get the pixel data format of this texture asset
         */
        [[nodiscard]] PixelFormat GetFormat() const;

    private:
        static constexpr uint8_t TEXTURE_ASSET_VERSION = 2;

        std::vector<uint8_t> pixelData{}; // just the bytes, NOT an array of pixels
        size_t width{};
        size_t height{};
        PixelFormat pixelFormat{};

        /**
        * Create an SDR @c TextureAsset from a PNG image
        * @param imagePath The path to the PNG
        * @return Error Code
        */
        [[nodiscard]] Error::ErrorCode CreateFromPNG(const std::string &imagePath);

        /**
         * Create an HDR @c TextureAsset from an EXR image
         * @param imagePath The path to the EXR
         * @return Error Code
         */
        [[nodiscard]] Error::ErrorCode CreateFromEXR(const std::string &imagePath);

        /**
         * Save this @c TextureAsset as a standard PNG image
         * @param imagePath The path to save to
         */
        [[nodiscard]] Error::ErrorCode SaveAsPNG(const std::string &imagePath) const;

        /**
         * Save this @c TextureAsset as a standard EXR image
         * @param imagePath The path to save to
         */
        [[nodiscard]] Error::ErrorCode SaveAsEXR(const std::string &imagePath) const;
};
