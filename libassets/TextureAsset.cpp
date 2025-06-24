//
// Created by droc101 on 6/23/25.
//

#include "include/libassets/TextureAsset.h"
#include <cassert>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <stb_image_write.h>

uint32_t TextureAsset::GetHeight() const
{
    return height;
}

uint32_t TextureAsset::GetWidth() const
{
    return width;
}

uint32_t *TextureAsset::GetPixels() const
{
    return pixels;
}

void TextureAsset::SaveAsImage(const char *imagePath, const ImageFormat format) const
{
    switch (format)
    {
        case IMAGE_FORMAT_PNG:
            stbi_write_png(imagePath, static_cast<int>(width), static_cast<int>(height), 4, pixels, static_cast<int>(width * sizeof(uint32_t)));
            break;
        case IMAGE_FORMAT_TGA:
            stbi_write_tga(imagePath, static_cast<int>(width), static_cast<int>(height), 4, pixels);
            break;
        case IMAGE_FORMAT_BMP:
            stbi_write_bmp(imagePath, static_cast<int>(width), static_cast<int>(height), 4, pixels);
            break;
        default:;
    }
}

TextureAsset::TextureAsset(const char *imagePath, int *channels)
{
    int width;
    int height;
    uint8_t *data = stbi_load(imagePath, &width, &height, channels, STBI_rgb_alpha);
    // assert(*channels == 4); // The image MUST be RGBA.
    this->width = width;
    this->height = height;
    this->pixels = new uint32_t[width * height];
    memcpy(this->pixels, data, width * height * sizeof(uint32_t));
    stbi_image_free(data);
}

TextureAsset::TextureAsset()
{
    this->width = 64;
    this->height = 64;
    constexpr std::size_t pixelDataSize = 64 * 64;
    pixels = new uint32_t[pixelDataSize];

    for (int x = 0; x < 64; x++)
    {
        for (int y = 0; y < 64; y++)
        {
            if ((x < 32) ^ (y < 32))
            {
                pixels[x + y * 64] = 0x000000ff; // black
            } else
            {
                pixels[x + y * 64] = 0xff00ffff; // magenta
            }
        }
    }
}

TextureAsset::TextureAsset(uint32_t *pixels, const uint32_t width, const uint32_t height)
{
    this->pixels = pixels;
    this->width = width;
    this->height = height;
}

void TextureAsset::FinishLoading()
{
    if (tempCompressedData != nullptr)
    {
        std::size_t decompressed_size = 0;
        uint8_t *data = Decompress(tempCompressedData, &decompressed_size);
        delete[] tempCompressedData;
        const uint32_t *header = reinterpret_cast<uint32_t *>(data);
        const size_t pixelDataSize = header[0];
        this->width = header[1];
        this->height = header[2];
        this->pixels = new uint32_t[this->width * this->height];
        memcpy(this->pixels, &header[4], pixelDataSize);
    }

    // "fix" the byte order
    for (int i = 0; i < this->width * this->height; i++)
    {
        const uint32_t pixel = this->pixels[i];
        const uint8_t a = static_cast<uint8_t>(pixel >> 24);
        const uint8_t r = static_cast<uint8_t>(pixel >> 16);
        const uint8_t g = static_cast<uint8_t>(pixel >> 8);
        const uint8_t b = static_cast<uint8_t>(pixel);

        this->pixels[i] = b << 24 | g << 16 | r << 8 | a;
    }
}

uint8_t *TextureAsset::SaveToBuffer(std::size_t *outSize)
{
    const size_t dataSize = sizeof(uint32_t) * (4 + (this->width * this->height));
    uint8_t *buffer = new uint8_t[dataSize];
    uint32_t *buffer32 = reinterpret_cast<uint32_t *>(buffer);
    buffer32[0] = this->width * this->height * sizeof(uint32_t);
    buffer32[1] = this->width;
    buffer32[2] = this->height;
    buffer32[3] = 0;
    for (int x = 0; x < this->width; x++)
    {
        for (int y = 0; y < this->height; y++)
        {
            const size_t arrayIndex = x + y * this->width;
            const uint32_t pixel = this->pixels[arrayIndex];

            const uint8_t a = static_cast<uint8_t>(pixel);
            const uint8_t r = static_cast<uint8_t>(pixel >> 8);
            const uint8_t g = static_cast<uint8_t>(pixel >> 16);
            const uint8_t b = static_cast<uint8_t>(pixel >> 24);

            buffer32[4 + arrayIndex] = a << 24 | r << 16 | g << 8 | b;
        }
    }
    *outSize = buffer32[0] + 16;
    return buffer;
}

