//
// Created by droc101 on 6/23/25.
//

#ifndef TEXTUREASSET_H
#define TEXTUREASSET_H
#include <cstdint>
#include "Asset.h"


class TextureAsset final: public Asset {

    using Asset::Asset;

    public:
        enum ImageFormat
        {
            IMAGE_FORMAT_PNG,
            IMAGE_FORMAT_TGA,
            IMAGE_FORMAT_BMP
        };

        /**
         * Create a texture asset from a pixel buffer
         * @param pixels The pixel buffer in RGBA 8-bit format
         * @param width The width of the image
         * @param height The height of the image
         */
        TextureAsset(uint32_t *pixels, uint32_t width, uint32_t height);

        /**
         * Create a texture asset from an image on disk
         * @param imagePath The path to the image on disk
         * @param channels Where to store the number of channels in the image
         */
        explicit TextureAsset(const char *imagePath, int *channels);

        /**
         * Create a texture asset with the "missing texture" texture
         */
        explicit TextureAsset();

        /**
         * Get an editable reference ot the pixels of this image.
         */
        [[nodiscard]] uint32_t *GetPixels() const;

        /**
         * Get a non-editable list of the image's pixels in RGBA format
         */
        [[nodiscard]] const uint32_t *GetPixelsRGBA() const;

        /**
         * Get the width of this image
         */
        [[nodiscard]] uint32_t GetWidth() const;
        /**
         * Get the height of this image
         */
        [[nodiscard]] uint32_t GetHeight() const;

        /**
         * Save this image to disk
         * @param imagePath The path to save to
         * @param format The format to save as
         */
        void SaveAsImage(const char *imagePath, ImageFormat format) const;

        void FinishLoading() override;

        uint8_t* SaveToBuffer(std::size_t *outSize) override;

    private:
        uint32_t *pixels;
        uint32_t width;
        uint32_t height;
};



#endif //TEXTUREASSET_H
