//
// Created by droc101 on 6/23/25.
//

#pragma once

#include <cstddef>
#include <cstdint>
#include <libassets/util/Error.h>
#include <vector>

class TextureAsset final
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
        TextureAsset() = default;

        /**
         * Create a @c TextureAsset from a .gtex asset
         * @param assetPath The path to the gtex file
         * @param texture The texture to load into
         * @return Error Code
         */
        [[nodiscard]] static Error::ErrorCode CreateFromAsset(const char *assetPath, TextureAsset &texture);

        /**
         * Create an SDR @c TextureAsset from a PNG image
         * @param imagePath The path to the PNG
         * @param texture The texture to load into
         * @return Error Code
         */
        [[nodiscard]] static Error::ErrorCode CreateFromPNG(const char *imagePath, TextureAsset &texture);

        /**
         * Create an HDR @c TextureAsset from an EXR image
         * @param imagePath The path to the EXR
         * @param texture The texture to load into
         * @return Error Code
         */
        [[nodiscard]] static Error::ErrorCode CreateFromEXR(const char *imagePath, TextureAsset &texture);

        /// Create a TextureAsset with the "missing texture" pattern
        static void CreateMissingTexture(TextureAsset &texture);

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

        /**
         * Save this @c TextureAsset as a standard PNG image
         * @param imagePath The path to save to
         */
        [[nodiscard]] Error::ErrorCode SaveAsPNG(const char *imagePath) const;

        /**
         * Save this @c TextureAsset as a standard EXR image
         * @param imagePath The path to save to
         */
        [[nodiscard]] Error::ErrorCode SaveAsEXR(const char *imagePath);

        /**
         * Save this @c TextureAsset as a GTEX file
         * @param assetPath The path to save to
         */
        [[nodiscard]] Error::ErrorCode SaveAsAsset(const char *assetPath) const;

        static constexpr uint8_t TEXTURE_ASSET_VERSION = 2;

        bool filter = false;
        bool repeat = true;
        bool mipmaps = true;

    private:
        std::vector<uint8_t> pixelData{}; // just the bytes, NOT an array of pixels
        size_t width{};
        size_t height{};
        PixelFormat pixelFormat{};

        /**
         * Create the uncompressed gtex payload
         * @param[out] buffer The output buffer to store the payload in
         */
        void SaveToBuffer(std::vector<uint8_t> &buffer) const;
};
