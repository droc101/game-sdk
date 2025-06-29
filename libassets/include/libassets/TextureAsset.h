//
// Created by droc101 on 6/23/25.
//

#ifndef TEXTUREASSET_H
#define TEXTUREASSET_H
#include <cstdint>
#include <vector>

class TextureAsset final {

    public:
        enum ImageFormat: uint8_t
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
         * @return @c The TextureAsset
         */
        [[nodiscard]] static TextureAsset CreateFromAsset(const char *assetPath);

        /**
         * Create a @c TextureAsset from a pixel buffer
         * @param pixels The pixel data
         * @param width The width of the texture
         * @param height The height of the texture
         * @return The @c TextureAsset
         */
        [[nodiscard]] static TextureAsset CreateFromPixels(uint32_t *pixels, uint32_t width, uint32_t height);

        /**
         * Create a @c TextureAsset from a conventional image file (such as PNG)
         * @param imagePath The path to the image file
         * @return The @c TextureAsset
         */
        [[nodiscard]] static TextureAsset CreateFromImage(const char *imagePath);

        /// Create a TextureAsset with the "missing texture" pattern
        [[nodiscard]] static TextureAsset CreateMissingTexture();

        /// Get the pixel data in RGBA format
        [[nodiscard]] const uint32_t *GetPixelsRGBA() const;

        /// Get the raw pixel data (not in RGBA)
        [[nodiscard]] const unsigned *GetPixels() const;

        /// Get the width of the texture
        [[nodiscard]] uint32_t GetWidth() const;

        /// Get the height of the texture
        [[nodiscard]] uint32_t GetHeight() const;

        /**
         * Save this @c TextureAsset as a conventional image (such as PNG)
         * @param imagePath The path to save to
         * @param format The format to save as
         */
        void SaveAsImage(const char *imagePath, ImageFormat format) const;

        /**
         * Save this @c TextureAsset as a GTEX file
         * @param assetPath The path to save to
         */
        void SaveAsAsset(const char *assetPath) const;

    private:
        std::vector<uint32_t> pixels;
        uint32_t width = 0;
        uint32_t height = 0;

        /**
         * Create the uncompressed gtex payload
         * @param outSize Where to store the size of the payload
         * @return The payload data
         */
        uint8_t *SaveToBuffer(size_t *outSize) const;

        /**
         * Fix the byte order on imported pixels
         */
        void FixByteOrder();
};



#endif //TEXTUREASSET_H
