//
// Created by droc101 on 6/23/25.
//

#include "include/libassets/AssetReader.h"
#include <cassert>
#include <csignal>
#include <cstdio>
#include <cstring>
#include <format>
#include <stdexcept>
#include <vector>
#include <zlib.h>
#include "include/libassets/DataReader.h"

AssetReader::AssetType AssetReader::Decompress(std::vector<uint8_t> &asset, DataReader &reader)
{
    assert(reader.bytes.empty());
    reader.offset = 0;
    constexpr size_t dataOffset = sizeof(uint32_t) * 4;
    assert(dataOffset <= asset.size());
    const uint32_t *header = reinterpret_cast<const uint32_t *>(asset.data());
    const uint32_t compressedSize = header[0];
    reader.size = header[1];
    // uint32_t*3 = unused
    const AssetType &assetType = static_cast<AssetType>(header[3]);

    reader.bytes.resize(reader.size);

    z_stream stream{};

    stream.next_in = asset.data() + dataOffset;
    stream.avail_in = compressedSize;
    stream.next_out = reader.bytes.data();
    stream.avail_out = reader.size;

    if (inflateInit2(&stream, MAX_WBITS | 16) != Z_OK)
    {
        throw std::runtime_error(std::format("inflateInit2() failed with error: {}",
                                             stream.msg == nullptr ? "(null)" : stream.msg));
    }

    int inflateReturnValue = inflate(&stream, Z_NO_FLUSH);
    while (inflateReturnValue != Z_STREAM_END)
    {
        if (inflateReturnValue != Z_OK)
        {
            throw std::runtime_error(std::format("inflate() failed with error: {}",
                                                 stream.msg == nullptr ? "(null)" : stream.msg));
        }
        inflateReturnValue = inflate(&stream, Z_NO_FLUSH);
    }

    if (inflateEnd(&stream) != Z_OK)
    {
        throw std::runtime_error(std::format("inflateEnd() failed with error: {}",
                                             stream.msg == nullptr ? "(null)" : stream.msg));
    }

    assert(reader.size == stream.total_out);

    return assetType;
}

void AssetReader::Compress(std::vector<uint8_t> &inBuffer, std::vector<uint8_t> &outBuffer, const AssetType type)
{
    assert(!inBuffer.empty());
    assert(outBuffer.empty());
    outBuffer.resize(sizeof(uint32_t) * 4);
    uint32_t *header = reinterpret_cast<uint32_t *>(outBuffer.data());
    header[1] = inBuffer.size();
    header[3] = static_cast<uint32_t>(type);

    z_stream zs{};
    deflateInit2(&zs, Z_BEST_COMPRESSION, Z_DEFLATED, 15 | 16, 8, Z_DEFAULT_STRATEGY);

    zs.next_in = inBuffer.data();
    zs.avail_in = inBuffer.size();

    int ret = Z_OK;
    constexpr size_t chunkSize = 16384;
    std::vector<uint8_t> readBuffer(chunkSize);

    while (ret == Z_OK)
    {
        zs.next_out = readBuffer.data();
        zs.avail_out = readBuffer.size();

        ret = deflate(&zs, Z_FINISH);

        const ptrdiff_t bytesCompressed = static_cast<ptrdiff_t>(readBuffer.size()) - zs.avail_out;
        outBuffer.insert(outBuffer.end(), readBuffer.begin(), readBuffer.begin() + bytesCompressed);
    }

    deflateEnd(&zs);

    if (ret != Z_STREAM_END)
    {
        throw std::runtime_error("deflate failed");
    }

    *reinterpret_cast<uint32_t *>(outBuffer.data()) = outBuffer.size() - sizeof(uint32_t) * 4;
}

void AssetReader::SaveToFile(const char *filePath, std::vector<uint8_t> &data, const AssetType type)
{
    FILE *file = fopen(filePath, "wb");
    if (file == nullptr)
    {
        throw std::runtime_error("Unable to open file for writing");
    }
    std::vector<uint8_t> compressedData;
    Compress(data, compressedData, type);
    fwrite(compressedData.data(), 1, compressedData.size(), file);
    fclose(file);
}

AssetReader::AssetType AssetReader::LoadFromFile(const char *filePath, DataReader &reader)
{
    std::FILE *file = std::fopen(filePath, "rb");
    if (file == nullptr)
    {
        throw std::runtime_error("Unable to open file");
    }
    fseek(file, 0, SEEK_END);
    const size_t dataSize = ftell(file);
    assert(dataSize >= sizeof(uint32_t) * 4);
    std::vector<uint8_t> compressedData(dataSize);
    fseek(file, 0, SEEK_SET);
    fread(compressedData.data(), 1, dataSize, file);
    fclose(file);
    return Decompress(compressedData, reader);
}
