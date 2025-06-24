//
// Created by droc101 on 6/23/25.
//

#include "include/libassets/Asset.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <vector>
#include <zlib.h>

Asset::Asset()
{
    asset_type = ASSET_TYPE_TEXTURE; // this is only here to please clang-tidy, it does not matter.
}

Asset::Asset(const char *path)
{
    std::FILE *file = std::fopen(path, "rb");
    if (file == nullptr)
    {
        throw std::runtime_error("Unable to open file");
    }
    fseek(file, 0, SEEK_END);
    const std::size_t data_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    uint8_t *data = new uint8_t[data_size];
    fread(data, 1, data_size, file);
    fclose(file);
    tempCompressedData = data;
    asset_type = ASSET_TYPE_TEXTURE; // this is only here to please clang-tidy, it does not matter.
}

Asset::Asset(uint8_t *data)
{
    tempCompressedData = data;
    asset_type = ASSET_TYPE_TEXTURE; // this is only here to please clang-tidy, it does not matter.
}

void Asset::FinishLoading()
{
    // std::size_t decompressed_size = 0;
    // uint8_t *data = Decompress(tempCompressedData, &decompressed_size);
    delete[] tempCompressedData;
}


uint8_t *Asset::Decompress(uint8_t *asset, std::size_t *outSize)
{
    constexpr std::size_t dataOffset = sizeof(uint32_t) * 4;
    const uint32_t *header = reinterpret_cast<const uint32_t *>(asset);
    const uint32_t compressedSize = header[0];
    const uint32_t decompressedSize = header[1];
    // uint32_t*3 = unused
    SetAssetType(static_cast<AssetType>(header[3]));

    uint8_t *decompressedData = new uint8_t[decompressedSize];

    z_stream stream = {nullptr};

    Bytef* asset_bytef = reinterpret_cast<Bytef*>(asset + dataOffset);
    stream.next_in = asset_bytef;
    stream.avail_in = compressedSize;
    stream.next_out = decompressedData;
    stream.avail_out = decompressedSize;

    assert(inflateInit2(&stream, MAX_WBITS | 16) == Z_OK);

    int inflateReturnValue = inflate(&stream, Z_NO_FLUSH);
    while (inflateReturnValue != Z_STREAM_END)
    {
        assert(inflateReturnValue == Z_OK);
        inflateReturnValue = inflate(&stream, Z_NO_FLUSH);
    }

    assert(inflateEnd(&stream) == Z_OK);

    if (outSize != nullptr)
    {
        *outSize = stream.total_out;
    }

    return decompressedData;
}

const uint8_t *Asset::Compress(uint8_t *data, std::size_t data_size) const
{
    uint32_t* header = new uint32_t[4];
    header[1] = data_size;
    header[3] = GetAssetType();

    std::vector<uint8_t> buffer;

    constexpr size_t BUFSIZE = 128 * 1024;
    uint8_t temp_buffer[BUFSIZE];

    Bytef* asset_bytef = reinterpret_cast<Bytef*>(data);

    z_stream strm;
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.next_in = asset_bytef;
    strm.avail_in = data_size;
    strm.next_out = temp_buffer;
    strm.avail_out = BUFSIZE;

    deflateInit(&strm, Z_BEST_COMPRESSION);

    while (strm.avail_in != 0)
    {
        const int res = deflate(&strm, Z_NO_FLUSH);
        assert(res == Z_OK);
        if (strm.avail_out == 0)
        {
            buffer.insert(buffer.end(), temp_buffer, temp_buffer + BUFSIZE);
            strm.next_out = temp_buffer;
            strm.avail_out = BUFSIZE;
        }
    }

    int deflate_res = Z_OK;
    while (deflate_res == Z_OK)
    {
        if (strm.avail_out == 0)
        {
            buffer.insert(buffer.end(), temp_buffer, temp_buffer + BUFSIZE);
            strm.next_out = temp_buffer;
            strm.avail_out = BUFSIZE;
        }
        deflate_res = deflate(&strm, Z_FINISH);
    }

    assert(deflate_res == Z_STREAM_END);
    buffer.insert(buffer.end(), temp_buffer, temp_buffer + BUFSIZE - strm.avail_out);
    deflateEnd(&strm);

    header[0] = buffer.size();

    uint8_t* output = new uint8_t[(sizeof(uint32_t) * 4) + buffer.size()];
    memcpy(output, header, sizeof(uint32_t) * 4);
    memcpy(output + sizeof(uint32_t) * 4, buffer.data(), buffer.size());

    delete header;

    return output;
}

void Asset::SaveToFile(const char *path)
{
    FILE *file = fopen(path, "wb");
    assert(file != nullptr);
    const uint8_t *buffer = SaveToBuffer();
    const uint32_t *header = reinterpret_cast<const uint32_t *>(buffer);
    const uint32_t compressedSize = header[0] + sizeof(uint32_t) * 4;
    fwrite(buffer, 1, compressedSize, file);
    fclose(file);
}

Asset::AssetType Asset::GetAssetType() const
{
    return asset_type;
}

void Asset::SetAssetType(AssetType assetType)
{
    asset_type = assetType;
}

uint8_t *Asset::SaveToBuffer()
{
    return nullptr;
}

