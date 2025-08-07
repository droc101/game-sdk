//
// Created by droc101 on 6/23/25.
//

#pragma once

#include <cstddef>
#include <cstdint>
#include <libassets/util/AssetReader.h>
#include <libassets/util/Error.h>
#include <vector>

class TextureAsset final
{
    public:
        enum class ImageFormat : uint8_t
        {
            IMAGE_FORMAT_PNG,
            IMAGE_FORMAT_TGA,
            IMAGE_FORMAT_BMP
        };

        /**
         * Please use @c TextureAsset::Create* instead.
         */
        TextureAsset() = default;

        /**
         * Create a @c TextureAsset from a .gtex asset
         * @param assetPath The path to the gtex file
         * @param texture
         * @return @c The TextureAsset
         */
        [[nodiscard]] static Error::ErrorCode CreateFromAsset(const char *assetPath, TextureAsset &texture);

        /**
         * Create a @c TextureAsset from a pixel buffer
         * @param pixels The pixel data
         * @param width The width of the texture
         * @param height The height of the texture
         * @param texture
         * @return The @c TextureAsset
         */
        [[nodiscard]] static Error::ErrorCode CreateFromPixels(uint32_t *pixels,
                                                               uint32_t width,
                                                               uint32_t height,
                                                               TextureAsset &texture);

        /**
         * Create a @c TextureAsset from a conventional image file (such as PNG)
         * @param imagePath The path to the image file
         * @param texture
         * @return The @c TextureAsset
         */
        [[nodiscard]] static Error::ErrorCode CreateFromImage(const char *imagePath, TextureAsset &texture);

        /// Create a TextureAsset with the "missing texture" pattern
        static void CreateMissingTexture(TextureAsset &texture);

        /// Get the pixel data in RGBA format
        void GetPixelsRGBA(std::vector<uint32_t> &outBuffer) const;

        /// Get the raw pixel data (not in RGBA)
        [[nodiscard]] unsigned *GetPixels();

        /// Get the width of the texture
        [[nodiscard]] uint32_t GetWidth() const;

        /// Get the height of the texture
        [[nodiscard]] uint32_t GetHeight() const;

        /**
         * Save this @c TextureAsset as a conventional image (such as PNG)
         * @param imagePath The path to save to
         * @param format The format to save as
         */
        [[nodiscard]] Error::ErrorCode SaveAsImage(const char *imagePath, ImageFormat format) const;

        /**
         * Save this @c TextureAsset as a GTEX file
         * @param assetPath The path to save to
         */
        [[nodiscard]] Error::ErrorCode SaveAsAsset(const char *assetPath) const;

        static constexpr uint8_t TEXTURE_ASSET_VERSION = 1;

        bool filter = false;
        bool repeat = true;
        bool mipmaps = true;

    private:
        std::vector<uint32_t> pixels{};
        size_t width{};
        size_t height{};

        /**
         * Create the uncompressed gtex payload
         * @param[out] buffer The output buffer to store the payload in
         */
        void SaveToBuffer(std::vector<uint8_t> &buffer) const;

        /**
         * Fix the byte order on imported pixels
         */
        void FixByteOrder();
};
