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
#include <libassets/util/AssetContainer.h>
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

TextureAsset::TextureAsset()
{
    Reset();
}

Asset::AssetType TextureAsset::GetAssetType() const
{
    return AssetType::ASSET_TYPE_TEXTURE;
}

uint8_t TextureAsset::GetAssetTypeVersion() const
{
    return TEXTURE_ASSET_VERSION;
}

void TextureAsset::Reset()
{
    width = 0;
    height = 0;
    filter = true;
    repeat = true;
    mipmaps = true;
    opaque = true;
    pixelFormat = PixelFormat::RGBA8;
    pixelData = {};
}

Error::ErrorCode TextureAsset::LoadFromBuffer(DataReader &reader)
{
    Reset();
    width = reader.Read<size_t>();
    height = reader.Read<size_t>();
    filter = reader.Read<uint8_t>() != 0;
    repeat = reader.Read<uint8_t>() != 0;
    mipmaps = reader.Read<uint8_t>() != 0;
    opaque = reader.Read<uint8_t>() != 0;
    pixelFormat = static_cast<PixelFormat>(reader.Read<uint8_t>());
    size_t pixelDataSize = width * height;
    if (pixelFormat == PixelFormat::RGBA8)
    {
        pixelDataSize *= 4; // 4 bytes
    } else
    {
        pixelDataSize *= 4 * 2; // 4 16-bit floats
    }
    reader.ReadToVector<uint8_t>(pixelData, pixelDataSize);
    return Error::ErrorCode::OK;
}

Error::ErrorCode TextureAsset::SaveToBuffer(DataWriter &writer) const
{
    writer.Write<size_t>(width);
    writer.Write<size_t>(height);
    writer.Write<uint8_t>(filter ? 1 : 0);
    writer.Write<uint8_t>(repeat ? 1 : 0);
    writer.Write<uint8_t>(mipmaps ? 1 : 0);
    writer.Write<uint8_t>(opaque ? 1 : 0);
    writer.Write<uint8_t>(static_cast<uint8_t>(pixelFormat));
    writer.WriteBuffer<uint8_t>(pixelData);
    return Error::ErrorCode::OK;
}

Error::ErrorCode TextureAsset::CreateFromPNG(const string &imagePath)
{
    if (access(imagePath.c_str(), F_OK | R_OK) != 0)
    {
        CreateMissingTexture();
        return Error::ErrorCode::OK;
    }
    int pngWidth = 0;
    int pngHeight = 0;
    int channels = 0;
    uint8_t *data = stbi_load(imagePath.c_str(), &pngWidth, &pngHeight, &channels, STBI_rgb_alpha);
    if (data == nullptr)
    {
        Logger::Error("stbi_load failed: {}", stbi_failure_reason());
        return Error::ErrorCode::UNKNOWN;
    }
    Reset();
    width = pngWidth;
    height = pngHeight;
    pixelFormat = PixelFormat::RGBA8;
    const size_t pixelDataSize = width * height * 4;
    pixelData = std::vector<uint8_t>(pixelDataSize);
    for (size_t i = 0; i < pixelDataSize; i++)
    {
        pixelData.at(i) = data[i];
    }
    stbi_image_free(data);

    opaque = true;
    for (size_t i = 3; i < pixelDataSize; i += 4)
    {
        if (pixelData.at(i) != std::numeric_limits<uint8_t>::max())
        {
            opaque = false;
            break;
        }
    }

    return Error::ErrorCode::OK;
}

Error::ErrorCode TextureAsset::CreateFromEXR(const string &imagePath)
{
    Reset();
    RgbaInputFile file = RgbaInputFile(imagePath.c_str());
    const Box2i dw = file.dataWindow();
    width = dw.max.x - dw.min.x + 1;
    height = dw.max.y - dw.min.y + 1;
    pixelData = std::vector<uint8_t>(width * height * 4 * 2);
    pixelFormat = PixelFormat::RGBAF16;
    file.setFrameBuffer(reinterpret_cast<Rgba *>(GetPixelsRGBA()), 1, width);
    file.readPixels(dw.min.y, dw.max.y);

    opaque = true;
    for (size_t i = 6; i < pixelData.size(); i += 8)
    {
        if (*reinterpret_cast<_Float16 *>(&pixelData.at(i)) < 1)
        {
            opaque = false;
            break;
        }
    }

    return Error::ErrorCode::OK;
}

void TextureAsset::CreateMissingTexture()
{
    Reset();
    filter = false;
    mipmaps = false;
    repeat = true;
    width = 64;
    height = 64;
    pixelFormat = PixelFormat::RGBA8;
    constexpr size_t PIXEL_DATA_SIZE = 64 * 64 * 4;
    pixelData = std::vector<uint8_t>(PIXEL_DATA_SIZE);
    uint32_t *pixels = reinterpret_cast<uint32_t *>(pixelData.data());

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

Error::ErrorCode TextureAsset::LoadFromAsset(const std::string &filePath)
{
    Reset();
    AssetContainer asset;
    const Error::ErrorCode error = AssetContainer::LoadFromFile(filePath.c_str(), asset);
    if (error != Error::ErrorCode::OK)
    {
        CreateMissingTexture();
        return Error::ErrorCode::OK;
    }
    if (asset.type != GetAssetType())
    {
        CreateMissingTexture();
        return Error::ErrorCode::OK;
    }
    if (asset.typeVersion != GetAssetTypeVersion())
    {
        CreateMissingTexture();
        return Error::ErrorCode::OK;
    }
    const Error::ErrorCode loadErr = LoadFromBuffer(asset.reader);
    if (loadErr != Error::ErrorCode::OK)
    {
        CreateMissingTexture();
    }
    return Error::ErrorCode::OK;
}

Error::ErrorCode TextureAsset::Import(const std::string &filePath)
{
    const std::filesystem::path path = filePath;
    const std::string extension = path.extension().string();
    if (extension == ".png")
    {
        return CreateFromPNG(filePath.c_str());
    }
    if (extension == ".exr")
    {
        return CreateFromEXR(filePath.c_str());
    }
    return Error::ErrorCode::INCORRECT_FORMAT;
}

Error::ErrorCode TextureAsset::Export(const std::string &filePath) const
{
    switch (pixelFormat)
    {
        case PixelFormat::RGBA8:
            return SaveAsPNG(filePath.c_str());
        case PixelFormat::RGBAF16:
            return SaveAsEXR(filePath.c_str());
    }
    return Error::ErrorCode::INCORRECT_FORMAT;
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

Error::ErrorCode TextureAsset::SaveAsPNG(const string &imagePath) const
{
    std::vector<uint8_t> pixelDataCopy = pixelData;
    const uint32_t *texturePixels = reinterpret_cast<uint32_t *>(pixelDataCopy.data());
    const int code = stbi_write_png(imagePath.c_str(),
                                    static_cast<int>(width),
                                    static_cast<int>(height),
                                    4,
                                    texturePixels,
                                    static_cast<int>(width * sizeof(uint32_t)));
    return code != 0 ? Error::ErrorCode::OK : Error::ErrorCode::UNKNOWN;
}

Error::ErrorCode TextureAsset::SaveAsEXR(const string &imagePath) const
{
    Header header = Header(static_cast<int>(width), static_cast<int>(height));
    header.channels().insert("R", Channel(HALF));
    header.channels().insert("G", Channel(HALF));
    header.channels().insert("B", Channel(HALF));
    header.channels().insert("A", Channel(HALF));

    FrameBuffer framebuffer;
    constexpr size_t PIXEL_SIZE = sizeof(half) * 4;
    // yup. i did a const cast :(
    char *base = const_cast<char *>(reinterpret_cast<const char *>(GetPixelsRGBA()));

    framebuffer.insert("R", Slice(HALF, base + sizeof(half) * 0, PIXEL_SIZE, PIXEL_SIZE * width));

    framebuffer.insert("G", Slice(HALF, base + sizeof(half) * 1, PIXEL_SIZE, PIXEL_SIZE * width));

    framebuffer.insert("B", Slice(HALF, base + sizeof(half) * 2, PIXEL_SIZE, PIXEL_SIZE * width));

    framebuffer.insert("A", Slice(HALF, base + sizeof(half) * 3, PIXEL_SIZE, PIXEL_SIZE * width));

    OutputFile file(imagePath.c_str(), header);
    file.setFrameBuffer(framebuffer);
    file.writePixels(static_cast<int>(height));
    return Error::ErrorCode::OK;
}

TextureAsset::PixelFormat TextureAsset::GetFormat() const
{
    return pixelFormat;
}

uint8_t *TextureAsset::GetPixelsRGBA()
{
    return pixelData.data();
}

const uint8_t *TextureAsset::GetPixelsRGBA() const
{
    return pixelData.data();
}
