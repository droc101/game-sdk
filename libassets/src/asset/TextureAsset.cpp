//
// Created by droc101 on 6/23/25.
//

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <libassets/asset/TextureAsset.h>
#include <libassets/util/Asset.h>
#include <libassets/util/AssetReader.h>
#include <libassets/util/DataReader.h>
#include <libassets/util/DataWriter.h>
#include <libassets/util/Error.h>
#include <vector>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <stb_image_write.h>

Error::ErrorCode TextureAsset::CreateFromImage(const char *imagePath, TextureAsset &texture)
{
    texture = TextureAsset();
    if (!std::filesystem::exists(imagePath))
    {
        CreateMissingTexture(texture);
        return Error::ErrorCode::OK;
    }
    int width = 0;
    int height = 0;
    int channels = 0;
    uint8_t *data = stbi_load(imagePath, &width, &height, &channels, STBI_rgb_alpha);
    const uint32_t *data32 = reinterpret_cast<uint32_t *>(data);
    texture.width = width;
    texture.height = height;
    texture.pixels = std::vector<uint32_t>(width * height);
    for (int i = 0; i < width * height; i++)
    {
        texture.pixels.at(i) = data32[i];
    }
    stbi_image_free(data);
    texture.FixByteOrder();
    return Error::ErrorCode::OK;
}

void TextureAsset::CreateMissingTexture(TextureAsset &texture)
{
    texture = TextureAsset();
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
}

Error::ErrorCode TextureAsset::CreateFromPixels(uint32_t *pixels,
                                                const uint32_t width,
                                                const uint32_t height,
                                                TextureAsset &texture)
{
    texture = TextureAsset();
    texture.width = width;
    texture.height = height;
    texture.pixels.insert(texture.pixels.begin(), pixels, pixels + texture.width * texture.height);
    return Error::ErrorCode::OK;
}

Error::ErrorCode TextureAsset::CreateFromAsset(const char *assetPath, TextureAsset &texture)
{
    texture = TextureAsset();
    if (!std::filesystem::exists(assetPath))
    {
        CreateMissingTexture(texture);
        return Error::ErrorCode::OK;
    }
    Asset asset;
    const Error::ErrorCode e = AssetReader::LoadFromFile(assetPath, asset);
    if (e != Error::ErrorCode::OK)
    {
        return e;
    }
    if (asset.type != Asset::AssetType::ASSET_TYPE_TEXTURE)
    {
        return Error::ErrorCode::INCORRECT_FORMAT;
    }
    if (asset.typeVersion != TEXTURE_ASSET_VERSION)
    {
        return Error::ErrorCode::INCORRECT_VERSION;
    }
    texture = TextureAsset();
    texture.width = asset.reader.Read<size_t>();
    texture.height = asset.reader.Read<size_t>();
    texture.filter = asset.reader.Read<uint8_t>() != 0;
    texture.repeat = asset.reader.Read<uint8_t>() != 0;
    texture.mipmaps = asset.reader.Read<uint8_t>() != 0;
    const size_t pixelCount = texture.width * texture.height;
    asset.reader.ReadToVector<uint32_t>(texture.pixels, pixelCount);
    return Error::ErrorCode::OK;
}

uint32_t TextureAsset::GetHeight() const
{
    return height;
}

uint32_t TextureAsset::GetWidth() const
{
    return width;
}

unsigned *TextureAsset::GetPixels()
{
    return pixels.data();
}

Error::ErrorCode TextureAsset::SaveAsImage(const char *imagePath, const ImageFormat format) const
{
    std::vector<uint32_t> pixels;
    GetPixelsRGBA(pixels);
    int code = 1; // default case fails
    switch (format)
    {
            using enum ImageFormat;
        case IMAGE_FORMAT_PNG:
            code = stbi_write_png(imagePath,
                                  static_cast<int>(width),
                                  static_cast<int>(height),
                                  4,
                                  pixels.data(),
                                  static_cast<int>(width * sizeof(uint32_t)));
            break;
        case IMAGE_FORMAT_TGA:
            code = stbi_write_tga(imagePath, static_cast<int>(width), static_cast<int>(height), 4, pixels.data());
            break;
        case IMAGE_FORMAT_BMP:
            code = stbi_write_bmp(imagePath, static_cast<int>(width), static_cast<int>(height), 4, pixels.data());
            break;
        default: ;
    }
    return code != 0 ? Error::ErrorCode::OK : Error::ErrorCode::UNKNOWN;
}

Error::ErrorCode TextureAsset::SaveAsAsset(const char *assetPath) const
{
    std::vector<uint8_t> buffer;
    SaveToBuffer(buffer);
    return AssetReader::SaveToFile(assetPath, buffer, Asset::AssetType::ASSET_TYPE_TEXTURE, TEXTURE_ASSET_VERSION);
}

void TextureAsset::SaveToBuffer(std::vector<uint8_t> &buffer) const
{
    DataWriter writer{};
    writer.Write<size_t>(width);
    writer.Write<size_t>(height);
    writer.Write<uint8_t>(filter ? 1 : 0);
    writer.Write<uint8_t>(repeat ? 1 : 0);
    writer.Write<uint8_t>(mipmaps ? 1 : 0);
    std::vector<uint32_t> pixels;
    GetPixelsRGBA(pixels);
    writer.WriteBuffer<uint32_t>(pixels);
    writer.CopyToVector(buffer);
}

void TextureAsset::GetPixelsRGBA(std::vector<uint32_t> &outBuffer) const
{
    outBuffer = pixels;
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
