//
// Created by droc101 on 6/23/25.
//

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <half.h>
#include <ImathBox.h>
#include <ImathConfig.h>
#include <ImfChannelList.h>
#include <ImfFrameBuffer.h>
#include <ImfHeader.h>
#include <ImfOutputFile.h>
#include <ImfPixelType.h>
#include <ImfRgba.h>
#include <ImfRgbaFile.h>
#include <libassets/asset/TextureAsset.h>
#include <libassets/type/Asset.h>
#include <libassets/util/AssetReader.h>
#include <libassets/util/DataReader.h>
#include <libassets/util/DataWriter.h>
#include <libassets/util/Error.h>
#include <libassets/util/Logger.h>
#include <OpenEXRConfig.h>
#include <vector>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <stb_image_write.h>

using namespace OPENEXR_IMF_NAMESPACE;
using namespace IMATH_NAMESPACE;

Error::ErrorCode TextureAsset::CreateFromPNG(const char *imagePath, TextureAsset &texture)
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
    if (data == nullptr)
    {
        Logger::Error("stbi_load failed: {}", stbi_failure_reason());
        return Error::ErrorCode::UNKNOWN;
    }
    texture.width = width;
    texture.height = height;
    texture.pixelFormat = PixelFormat::RGBA8;
    const size_t pixelDataSize = width * height * 4;
    texture.pixelData = std::vector<uint8_t>(pixelDataSize);
    for (size_t i = 0; i < pixelDataSize; i++)
    {
        texture.pixelData.at(i) = data[i];
    }
    stbi_image_free(data);
    return Error::ErrorCode::OK;
}

Error::ErrorCode TextureAsset::CreateFromEXR(const char *imagePath, TextureAsset &texture)
{
    RgbaInputFile file = RgbaInputFile(imagePath);
    const Box2i dw = file.dataWindow();
    texture.width = dw.max.x - dw.min.x + 1;
    texture.height = dw.max.y - dw.min.y + 1;
    texture.pixelData = std::vector<uint8_t>(texture.width * texture.height * 4 * 2);
    texture.pixelFormat = PixelFormat::RGBAF16;
    file.setFrameBuffer(reinterpret_cast<Rgba *>(texture.GetPixelsRGBA()), 1, texture.width);
    file.readPixels(dw.min.y, dw.max.y);
    return Error::ErrorCode::OK;
}

void TextureAsset::CreateMissingTexture(TextureAsset &texture)
{
    texture = TextureAsset();
    texture.width = 64;
    texture.height = 64;
    texture.pixelFormat = PixelFormat::RGBA8;
    constexpr size_t PIXEL_DATA_SIZE = 64 * 64 * 4;
    texture.pixelData = std::vector<uint8_t>(PIXEL_DATA_SIZE);
    uint32_t *pixels = reinterpret_cast<uint32_t *>(texture.pixelData.data());

    for (int x = 0; x < 64; x++)
    {
        for (int y = 0; y < 64; y++)
        {
            if ((x < 32) ^ (y < 32))
            {
                pixels[x + y * 64] = 0xff000000; // black
            } else
            {
                pixels[x + y * 64] = 0xffff00ff; // magenta
            }
        }
    }
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
        CreateMissingTexture(texture);
        return e;
    }
    if (asset.type != Asset::AssetType::ASSET_TYPE_TEXTURE)
    {
        CreateMissingTexture(texture);
        return Error::ErrorCode::INCORRECT_FORMAT;
    }
    if (asset.typeVersion != TEXTURE_ASSET_VERSION)
    {
        CreateMissingTexture(texture);
        return Error::ErrorCode::INCORRECT_VERSION;
    }
    texture = TextureAsset();
    texture.width = asset.reader.Read<size_t>();
    texture.height = asset.reader.Read<size_t>();
    texture.filter = asset.reader.Read<uint8_t>() != 0;
    texture.repeat = asset.reader.Read<uint8_t>() != 0;
    texture.mipmaps = asset.reader.Read<uint8_t>() != 0;
    texture.pixelFormat = static_cast<PixelFormat>(asset.reader.Read<uint8_t>());
    size_t pixelDataSize = texture.width * texture.height;
    if (texture.pixelFormat == PixelFormat::RGBA8)
    {
        pixelDataSize *= 4; // 4 bytes
    } else
    {
        pixelDataSize *= 4 * 2; // 4 16-bit floats
    }
    asset.reader.ReadToVector<uint8_t>(texture.pixelData, pixelDataSize);
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

size_t TextureAsset::GetPixelDataSize() const
{
    return pixelData.size();
}

Error::ErrorCode TextureAsset::SaveAsPNG(const char *imagePath) const
{
    std::vector<uint8_t> pixelDataCopy = pixelData;
    uint32_t *texturePixels = reinterpret_cast<uint32_t *>(pixelDataCopy.data());
    for (size_t i = 0; i < width * height; i++)
    {
        uint32_t *pixel = &texturePixels[i];
        const uint8_t a = static_cast<uint8_t>(*pixel >> 24);
        const uint8_t r = static_cast<uint8_t>(*pixel >> 16);
        const uint8_t g = static_cast<uint8_t>(*pixel >> 8);
        const uint8_t b = static_cast<uint8_t>(*pixel);

        *pixel = b << 24 | g << 16 | r << 8 | a;
    }
    const int code = stbi_write_png(imagePath,
                                    static_cast<int>(width),
                                    static_cast<int>(height),
                                    4,
                                    texturePixels,
                                    static_cast<int>(width * sizeof(uint32_t)));
    return code != 0 ? Error::ErrorCode::OK : Error::ErrorCode::UNKNOWN;
}

Error::ErrorCode TextureAsset::SaveAsEXR(const char *imagePath)
{
    Header header = Header(static_cast<int>(width), static_cast<int>(height));
    header.channels().insert("R", Channel(HALF));
    header.channels().insert("G", Channel(HALF));
    header.channels().insert("B", Channel(HALF));
    header.channels().insert("A", Channel(HALF));

    FrameBuffer framebuffer;
    constexpr size_t PIXEL_SIZE = sizeof(half) * 4;
    char *base = reinterpret_cast<char *>(GetPixelsRGBA());

    framebuffer.insert("R", Slice(HALF, base + sizeof(half) * 0, PIXEL_SIZE, PIXEL_SIZE * width));

    framebuffer.insert("G", Slice(HALF, base + sizeof(half) * 1, PIXEL_SIZE, PIXEL_SIZE * width));

    framebuffer.insert("B", Slice(HALF, base + sizeof(half) * 2, PIXEL_SIZE, PIXEL_SIZE * width));

    framebuffer.insert("A", Slice(HALF, base + sizeof(half) * 3, PIXEL_SIZE, PIXEL_SIZE * width));

    OutputFile file(imagePath, header);
    file.setFrameBuffer(framebuffer);
    file.writePixels(static_cast<int>(height));
    return Error::ErrorCode::OK;
}

Error::ErrorCode TextureAsset::SaveAsAsset(const char *assetPath) const
{
    std::vector<uint8_t> buffer;
    SaveToBuffer(buffer);
    return AssetReader::SaveToFile(assetPath,
                                   buffer,
                                   Asset::AssetType::ASSET_TYPE_TEXTURE,
                                   TEXTURE_ASSET_VERSION,
                                   AssetReader::BEST_COMPRESSION);
}

TextureAsset::PixelFormat TextureAsset::GetFormat() const
{
    return pixelFormat;
}

void TextureAsset::SaveToBuffer(std::vector<uint8_t> &buffer) const
{
    DataWriter writer{};
    writer.Write<size_t>(width);
    writer.Write<size_t>(height);
    writer.Write<uint8_t>(filter ? 1 : 0);
    writer.Write<uint8_t>(repeat ? 1 : 0);
    writer.Write<uint8_t>(mipmaps ? 1 : 0);
    writer.Write<uint8_t>(static_cast<uint8_t>(pixelFormat));
    writer.WriteBuffer<uint8_t>(pixelData);
    writer.CopyToVector(buffer);
}

uint8_t *TextureAsset::GetPixelsRGBA()
{
    return pixelData.data();
}

const uint8_t *TextureAsset::GetPixelsRGBA() const
{
    return pixelData.data();
}
