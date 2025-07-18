//
// Created by droc101 on 6/23/25.
//

#include <libassets/asset/TextureAsset.h>
#include <cassert>
#include <filesystem>
#include <libassets/util/AssetReader.h>
#include <libassets/util/DataReader.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <stb_image_write.h>

TextureAsset TextureAsset::CreateFromImage(const char *imagePath)
{
    if (!std::filesystem::exists(imagePath))
    {
        return CreateMissingTexture();
    }
    TextureAsset texture;
    int width;
    int height;
    int channels;
    uint8_t *data = stbi_load(imagePath, &width, &height, &channels, STBI_rgb_alpha);
    const uint32_t *data32 = reinterpret_cast<uint32_t *>(data);
    // assert(*channels == 4); // The image MUST be RGBA.
    texture.width = width;
    texture.height = height;
    texture.pixels = std::vector<uint32_t>(width * height);
    for (int i = 0; i < width * height; i++)
    {
        texture.pixels.at(i) = data32[i];
    }
    stbi_image_free(data);
    texture.FixByteOrder();
    return texture;
}

TextureAsset TextureAsset::CreateMissingTexture()
{
    TextureAsset texture;
    texture.width = 64;
    texture.height = 64;
    constexpr size_t pixelDataSize = 64 * 64;
    texture.pixels = std::vector<uint32_t>(pixelDataSize);

    for (int x = 0; x < 64; x++)
    {
        for (int y = 0; y < 64; y++)
        {
            if ((x < 32) ^ (y < 32))
            {
                texture.pixels.at(x + y * 64) = 0x000000ff; // black
            } else
            {
                texture.pixels.at(x + y * 64) = 0xff00ffff; // magenta
            }
        }
    }
    return texture;
}

TextureAsset TextureAsset::CreateFromPixels(uint32_t *pixels, const uint32_t width, const uint32_t height)
{
    TextureAsset texture;
    texture.width = width;
    texture.height = height;
    texture.pixels.insert(texture.pixels.begin(), pixels, pixels + texture.width * texture.height);
    return texture;
}

TextureAsset TextureAsset::CreateFromAsset(const char *assetPath)
{
    if (!std::filesystem::exists(assetPath))
    {
        return CreateMissingTexture();
    }
    DataReader reader;
    [[maybe_unused]] const AssetReader::AssetType assetType = AssetReader::LoadFromFile(assetPath, reader);
    assert(assetType == AssetReader::AssetType::ASSET_TYPE_TEXTURE);
    TextureAsset texture;
    reader.Seek(sizeof(uint32_t)); // 0 = pixelDataSize in bytes
    texture.width = reader.Read<uint32_t>();
    texture.height = reader.Read<uint32_t>();
    reader.Seek(sizeof(uint32_t)); // 3 = unused (3 sucks)
    const size_t pixelCount = texture.width * texture.height;
    reader.ReadToBuffer<uint32_t>(texture.pixels, pixelCount);
    texture.FixByteOrder();
    return texture;
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
    std::vector<uint32_t> pixels;
    GetPixelsRGBA(pixels);
    switch (format)
    {
        using enum ImageFormat;
        case IMAGE_FORMAT_PNG:
            stbi_write_png(imagePath,
                           static_cast<int>(width),
                           static_cast<int>(height),
                           4,
                           pixels.data(),
                           static_cast<int>(width * sizeof(uint32_t)));
            break;
        case IMAGE_FORMAT_TGA:
            stbi_write_tga(imagePath, static_cast<int>(width), static_cast<int>(height), 4, pixels.data());
            break;
        case IMAGE_FORMAT_BMP:
            stbi_write_bmp(imagePath, static_cast<int>(width), static_cast<int>(height), 4, pixels.data());
            break;
        default:;
    }
}

void TextureAsset::SaveAsAsset(const char *assetPath) const
{
    std::vector<uint8_t> buffer;
    SaveToBuffer(buffer);
    AssetReader::SaveToFile(assetPath, buffer, AssetReader::AssetType::ASSET_TYPE_TEXTURE);
}


void TextureAsset::FixByteOrder()
{
    assert(pixels.size() == width * height);
    for (uint32_t &pixel: pixels)
    {
        const uint8_t a = static_cast<uint8_t>(pixel >> 24);
        const uint8_t r = static_cast<uint8_t>(pixel >> 16);
        const uint8_t g = static_cast<uint8_t>(pixel >> 8);
        const uint8_t b = static_cast<uint8_t>(pixel);

        pixel = b << 24 | g << 16 | r << 8 | a;
    }
}

void TextureAsset::SaveToBuffer(std::vector<uint8_t> &buffer) const
{
    assert(buffer.empty());
    buffer.resize(sizeof(uint32_t) * 4);
    uint32_t *bufferAsUint32 = reinterpret_cast<uint32_t *>(buffer.data());
    bufferAsUint32[0] = width * height * sizeof(uint32_t);
    bufferAsUint32[1] = width;
    bufferAsUint32[2] = height;
    bufferAsUint32[3] = 0;
    std::vector<uint32_t> pixels;
    GetPixelsRGBA(pixels);
    buffer.insert(buffer.end(), pixels.begin(), pixels.end());
}

void TextureAsset::GetPixelsRGBA(std::vector<uint32_t> &outBuffer) const
{
    assert(outBuffer.empty());
    outBuffer.resize(width * height);
    for (uint32_t x = 0; x < width; x++)
    {
        for (uint32_t y = 0; y < height; y++)
        {
            const size_t arrayIndex = x + y * width;
            const uint32_t pixel = pixels.at(arrayIndex);

            const uint8_t a = static_cast<uint8_t>(pixel);
            const uint8_t r = static_cast<uint8_t>(pixel >> 8);
            const uint8_t g = static_cast<uint8_t>(pixel >> 16);
            const uint8_t b = static_cast<uint8_t>(pixel >> 24);

            outBuffer.at(arrayIndex) = a << 24 | r << 16 | g << 8 | b;
        }
    }
}
