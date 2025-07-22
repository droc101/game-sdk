//
// Created by droc101 on 6/23/25.
//

#include <libassets/util/AssetReader.h>
#include <csignal>
#include <cstdio>
#include <format>
#include <vector>
#include <zlib.h>
#include <libassets/util/DataReader.h>

Error::ErrorCode AssetReader::Decompress(std::vector<uint8_t> &asset, Asset &outAsset)
{
    if (!outAsset.reader.bytes.empty())
    {
        return Error::ErrorCode::E_INVALID_ARGUMENT;
    }
    outAsset.reader.offset = 0;
    constexpr size_t dataOffset = sizeof(uint32_t) * 4;
    if (dataOffset > asset.size())
    {
        return Error::ErrorCode::E_INVALID_HEADER;
    }
    const uint32_t *header = reinterpret_cast<const uint32_t *>(asset.data());
    const uint32_t compressedSize = header[0];
    outAsset.reader.size = header[1];
    // uint32_t*3 = unused
    const Asset::AssetType &assetType = static_cast<Asset::AssetType>(header[3]);

    outAsset.reader.bytes.resize(outAsset.reader.size);

    z_stream stream{};

    stream.next_in = asset.data() + dataOffset;
    stream.avail_in = compressedSize;
    stream.next_out = outAsset.reader.bytes.data();
    stream.avail_out = outAsset.reader.size;

    if (inflateInit2(&stream, MAX_WBITS | 16) != Z_OK)
    {
        printf(std::format("inflateInit2() failed with error: {}",
                           stream.msg == nullptr ? "(null)" : stream.msg).c_str());
        return Error::ErrorCode::E_COMPRESSION_ERROR;
    }

    int inflateReturnValue = inflate(&stream, Z_NO_FLUSH);
    while (inflateReturnValue != Z_STREAM_END)
    {
        if (inflateReturnValue != Z_OK)
        {
            printf(std::format("inflate() failed with error: {}",
                               stream.msg == nullptr ? "(null)" : stream.msg).c_str());
            return Error::ErrorCode::E_COMPRESSION_ERROR;
        }
        inflateReturnValue = inflate(&stream, Z_NO_FLUSH);
    }

    if (inflateEnd(&stream) != Z_OK)
    {
        printf(std::format("inflateEnd() failed with error: {}",
                           stream.msg == nullptr ? "(null)" : stream.msg).c_str());
        return Error::ErrorCode::E_COMPRESSION_ERROR;
    }

    if (outAsset.reader.size != stream.total_out)
    {
        return Error::ErrorCode::E_INVALID_BODY;
    }

    outAsset.type = assetType;
    return Error::ErrorCode::E_OK;
}

Error::ErrorCode AssetReader::Compress(std::vector<uint8_t> &inBuffer, std::vector<uint8_t> &outBuffer,
                                       const Asset::AssetType type)
{
    if (inBuffer.empty())
    {
        return Error::ErrorCode::E_INVALID_ARGUMENT;
    }
    if (!outBuffer.empty())
    {
        return Error::ErrorCode::E_INVALID_ARGUMENT;
    }
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
        printf("deflate failed");
        return Error::ErrorCode::E_COMPRESSION_ERROR;
    }

    *reinterpret_cast<uint32_t *>(outBuffer.data()) = outBuffer.size() - sizeof(uint32_t) * 4;

    return Error::ErrorCode::E_OK;
}

Error::ErrorCode AssetReader::SaveToFile(const char *filePath, std::vector<uint8_t> &data,
                                         const Asset::AssetType type)
{
    FILE *file = fopen(filePath, "wb");
    if (file == nullptr)
    {
        printf("Unable to open file for writing");
        return Error::ErrorCode::E_CANT_OPEN_FILE;
    }
    std::vector<uint8_t> compressedData;
    const Error::ErrorCode e = Compress(data, compressedData, type);
    fwrite(compressedData.data(), 1, compressedData.size(), file);
    fclose(file);
    return e;
}

Error::ErrorCode AssetReader::LoadFromFile(const char *filePath, Asset &outAsset)
{
    std::FILE *file = std::fopen(filePath, "rb");
    if (file == nullptr)
    {
        return Error::ErrorCode::E_FILE_NOT_FOUND;
    }
    fseek(file, 0, SEEK_END);
    const size_t dataSize = ftell(file);
    if (dataSize < sizeof(uint32_t) * 4)
    {
        return Error::ErrorCode::E_INVALID_HEADER;
    }
    std::vector<uint8_t> compressedData(dataSize);
    fseek(file, 0, SEEK_SET);
    fread(compressedData.data(), 1, dataSize, file);
    fclose(file);
    return Decompress(compressedData, outAsset);
}
