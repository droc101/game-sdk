//
// Created by droc101 on 6/23/25.
//

#include "include/libassets/TextureAsset.h"
#include <cassert>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <stb_image_write.h>
#include "include/libassets/AssetReader.h"

TextureAsset TextureAsset::CreateFromImage(const char *imagePath)
{
    TextureAsset t = TextureAsset();
    int width;
    int height;
    int channels;
    uint8_t *data = stbi_load(imagePath, &width, &height, &channels, STBI_rgb_alpha);
    const uint32_t *data32 = reinterpret_cast<uint32_t *>(data);
    // assert(*channels == 4); // The image MUST be RGBA.
    t.width = width;
    t.height = height;
    t.pixels = std::vector<uint32_t>(width*height);
    for (int i = 0; i < width*height; i++)
    {
        t.pixels[i] = data32[i];
    }
    stbi_image_free(data);
    t.FixByteOrder();
    return t;
}

TextureAsset TextureAsset::CreateMissingTexture()
{
    TextureAsset t = TextureAsset();
    t.width = 64;
    t.height = 64;
    constexpr std::size_t pixelDataSize = 64 * 64;
    t.pixels = std::vector<uint32_t>(pixelDataSize);

    for (int x = 0; x < 64; x++)
    {
        for (int y = 0; y < 64; y++)
        {
            if ((x < 32) ^ (y < 32))
            {
                t.pixels[x + y * 64] = 0x000000ff; // black
            } else
            {
                t.pixels[x + y * 64] = 0xff00ffff; // magenta
            }
        }
    }
    t.FixByteOrder();
    return t;
}

TextureAsset TextureAsset::CreateFromPixels(uint32_t *pixels, const uint32_t width, const uint32_t height)
{
    TextureAsset t;
    t.width = width;
    t.height = height;
    t.pixels = std::vector<uint32_t>();
    t.pixels.insert(t.pixels.end(), &pixels[0], &pixels[t.width * t.height]);
    t.FixByteOrder();
    return t;
}

TextureAsset TextureAsset::CreateFromAsset(const char *assetPath)
{
    std::size_t aSz;
    AssetReader::AssetType aTp;
    const uint8_t *data = AssetReader::LoadFromFile(assetPath, &aSz, &aTp);
    assert(aSz >= sizeof(uint32_t) * 4);
    assert(aTp == AssetReader::ASSET_TYPE_TEXTURE);
    const uint32_t *data32 = reinterpret_cast<const uint32_t*>(data);
    TextureAsset t = TextureAsset();
    // 0 = pixelDataSize in bytes
    t.width = data32[1];
    t.height = data32[2];
    // 3 = unused (3 sucks)
    const std::size_t pixelCount = t.width * t.height;
    assert(aSz >= sizeof(uint32_t) * (4 + pixelCount));
    t.pixels = std::vector<uint32_t>();
    t.pixels.insert(t.pixels.end(), &data32[4], &data32[4 + pixelCount]);
    t.FixByteOrder();
    delete[] data;
    return t;
}

uint32_t TextureAsset::GetHeight() const
{
    return height;
}

uint32_t TextureAsset::GetWidth() const
{
    return width;
}

const unsigned *TextureAsset::GetPixels() const
{
    return pixels.data();
}

void TextureAsset::SaveAsImage(const char *imagePath, const ImageFormat format) const
{
    const uint32_t *rgba = GetPixelsRGBA();
    switch (format)
    {
        case IMAGE_FORMAT_PNG:
            stbi_write_png(imagePath, static_cast<int>(width), static_cast<int>(height), 4, rgba, static_cast<int>(width * sizeof(uint32_t)));
            break;
        case IMAGE_FORMAT_TGA:
            stbi_write_tga(imagePath, static_cast<int>(width), static_cast<int>(height), 4, rgba);
            break;
        case IMAGE_FORMAT_BMP:
            stbi_write_bmp(imagePath, static_cast<int>(width), static_cast<int>(height), 4, rgba);
            break;
        default:;
    }
    delete[] rgba;
}

void TextureAsset::SaveAsAsset(const char *assetPath) const
{
    std::size_t size;
    uint8_t *buffer = SaveToBuffer(&size);
    AssetReader::SaveToFile(assetPath, buffer, size, AssetReader::ASSET_TYPE_TEXTURE);
    delete[] buffer;
}


void TextureAsset::FixByteOrder()
{
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

uint8_t *TextureAsset::SaveToBuffer(std::size_t *outSize) const
{
    const size_t dataSize = sizeof(uint32_t) * (4 + (this->width * this->height));
    uint8_t *buffer = new uint8_t[dataSize];
    uint32_t *buffer32 = reinterpret_cast<uint32_t *>(buffer);
    buffer32[0] = this->width * this->height * sizeof(uint32_t);
    buffer32[1] = this->width;
    buffer32[2] = this->height;
    buffer32[3] = 0;
    const uint32_t *rgba = GetPixelsRGBA();
    memcpy(buffer32 + 4, rgba, sizeof(uint32_t) * GetWidth() * GetHeight());
    *outSize = buffer32[0] + 16;
    delete[] rgba;
    return buffer;
}

const uint32_t *TextureAsset::GetPixelsRGBA() const
{
    uint32_t *pixelsRGBA = new uint32_t[this->width * this->height];
    for (int x = 0; x < this->width; x++)
    {
        for (int y = 0; y < this->height; y++)
        {
            const size_t arrayIndex = x + y * this->width;
            const uint32_t pixel = this->pixels.at(arrayIndex);

            const uint8_t a = static_cast<uint8_t>(pixel);
            const uint8_t r = static_cast<uint8_t>(pixel >> 8);
            const uint8_t g = static_cast<uint8_t>(pixel >> 16);
            const uint8_t b = static_cast<uint8_t>(pixel >> 24);

            pixelsRGBA[arrayIndex] = a << 24 | r << 16 | g << 8 | b;
        }
    }
    return pixelsRGBA;
}

