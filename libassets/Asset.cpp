//
// Created by droc101 on 6/23/25.
//

#include "include/libassets/Asset.h"
#include <cassert>
#include <csignal>
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

    if (inflateInit2(&stream, MAX_WBITS | 16) != Z_OK)
    {
        printf("inflateInit2() failed with error: %s", stream.msg);
        fflush(stdout);
        raise(SIGABRT);
    }

    int inflateReturnValue = inflate(&stream, Z_NO_FLUSH);
    while (inflateReturnValue != Z_STREAM_END)
    {
        if (inflateReturnValue != Z_OK)
        {
            printf("inflate() failed with error: %s", stream.msg);
            fflush(stdout);
            raise(SIGABRT);
        }
        inflateReturnValue = inflate(&stream, Z_NO_FLUSH);
    }

    if (inflateEnd(&stream) != Z_OK)
    {
        printf("inflateEnd() failed with error: %s", stream.msg);
        fflush(stdout);
        raise(SIGABRT);
    }

    if (outSize != nullptr)
    {
        *outSize = stream.total_out;
    }

    return decompressedData;
}

const uint8_t *Asset::Compress(uint8_t *data, std::size_t data_size, std::size_t *out_compressed_size) const
{
    uint32_t* header = new uint32_t[4];
    header[1] = data_size;
    header[3] = GetAssetType();

    Bytef* asset_bytef = reinterpret_cast<Bytef*>(data);

    z_stream zs{};
    deflateInit2(&zs, Z_BEST_COMPRESSION, Z_DEFLATED, 15 | 16, 8, Z_DEFAULT_STRATEGY);

    zs.next_in = asset_bytef;
    zs.avail_in = data_size;

    int ret;
    constexpr size_t chunkSize = 16384;
    std::vector<uint8_t> outBuffer(chunkSize);
    std::vector<uint8_t> compressedData;

    do {
        zs.next_out = outBuffer.data();
        zs.avail_out = outBuffer.size();

        ret = deflate(&zs, Z_FINISH);

        const size_t bytesCompressed = outBuffer.size() - zs.avail_out;
        compressedData.insert(compressedData.end(), outBuffer.begin(), outBuffer.begin() + bytesCompressed);
    } while (ret == Z_OK);

    deflateEnd(&zs);

    if (ret != Z_STREAM_END) {
        throw std::runtime_error("deflate failed");
    }

    header[0] = compressedData.size();

    uint8_t* output = new uint8_t[(sizeof(uint32_t) * 4) + compressedData.size()];
    memcpy(output, header, sizeof(uint32_t) * 4);
    memcpy(output + sizeof(uint32_t) * 4, compressedData.data(),compressedData.size());

    delete[] header;

    *out_compressed_size = compressedData.size();
    return output;
}

void Asset::SaveToFile(const char *path)
{
    FILE *file = fopen(path, "wb");
    assert(file != nullptr);
    std::size_t size;
    uint8_t *rawBuffer = SaveToBuffer(&size);
    std::size_t compressedSize;
    const uint8_t *buffer = Compress(rawBuffer, size, &compressedSize);
    fwrite(buffer, 1, compressedSize + 16, file);
    fclose(file);
    delete[] rawBuffer;
    delete[] buffer;
}

Asset::AssetType Asset::GetAssetType() const
{
    return asset_type;
}

void Asset::SetAssetType(AssetType assetType)
{
    asset_type = assetType;
}

uint8_t *Asset::SaveToBuffer(std::size_t *outSize)
{
    *outSize = 0;
    return nullptr;
}

