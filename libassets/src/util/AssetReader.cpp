//
// Created by droc101 on 6/23/25.
//

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <libassets/type/Asset.h>
#include <libassets/util/AssetReader.h>
#include <libassets/util/DataReader.h>
#include <libassets/util/DataWriter.h>
#include <libassets/util/Error.h>
#include <vector>
#include <zconf.h>
#include <zlib.h>

Error::ErrorCode AssetReader::Decompress(std::vector<uint8_t> &asset, Asset &outAsset)
{
    if (!outAsset.reader.bytes.empty())
    {
        return Error::ErrorCode::INVALID_ARGUMENT;
    }
    outAsset.reader.offset = 0;
    if (Asset::ASSET_HEADER_SIZE > asset.size())
    {
        return Error::ErrorCode::INVALID_HEADER;
    }

    DataReader reader = DataReader(asset);
    const uint32_t magic = reader.Read<uint32_t>();
    if (magic != Asset::ASSET_CONTAINER_MAGIC)
    {
        return Error::ErrorCode::INVALID_HEADER;
    }
    const uint8_t version = reader.Read<uint8_t>();
    if (version != Asset::ASSET_CONTAINER_VERSION)
    {
        return Error::ErrorCode::INCORRECT_VERSION;
    }
    outAsset.type = static_cast<Asset::AssetType>(reader.Read<uint8_t>());
    outAsset.typeVersion = reader.Read<uint8_t>();
    const size_t decompressedSize = reader.Read<size_t>();
    const size_t compressedSize = reader.Read<size_t>();

    outAsset.reader.size = decompressedSize;

    outAsset.reader.bytes.resize(outAsset.reader.size);

    z_stream zStream{};

    zStream.next_in = asset.data() + Asset::ASSET_HEADER_SIZE;
    zStream.avail_in = compressedSize;
    zStream.next_out = outAsset.reader.bytes.data();
    zStream.avail_out = outAsset.reader.size;
    zStream.data_type = Z_BINARY;

    if (inflateInit2(&zStream, MAX_WBITS | 16) != Z_OK)
    {
        printf("inflateInit2() failed with error: %s", zStream.msg == nullptr ? "(null)" : zStream.msg);
        return Error::ErrorCode::COMPRESSION_ERROR;
    }

    int inflateReturnValue = inflate(&zStream, Z_NO_FLUSH);
    while (inflateReturnValue != Z_STREAM_END)
    {
        if (inflateReturnValue != Z_OK)
        {
            printf("inflate() failed with error: %s", zStream.msg == nullptr ? "(null)" : zStream.msg);
            return Error::ErrorCode::COMPRESSION_ERROR;
        }
        inflateReturnValue = inflate(&zStream, Z_NO_FLUSH);
    }

    if (inflateEnd(&zStream) != Z_OK)
    {
        printf("inflateEnd() failed with error: %s", zStream.msg == nullptr ? "(null)" : zStream.msg);
        return Error::ErrorCode::COMPRESSION_ERROR;
    }

    if (outAsset.reader.size != zStream.total_out)
    {
        return Error::ErrorCode::INVALID_BODY;
    }
    return Error::ErrorCode::OK;
}

Error::ErrorCode AssetReader::Compress(std::vector<uint8_t> &inBuffer,
                                       std::vector<uint8_t> &outBuffer,
                                       const Asset::AssetType type,
                                       const uint8_t typeVersion)
{
    if (inBuffer.empty())
    {
        return Error::ErrorCode::INVALID_ARGUMENT;
    }
    if (!outBuffer.empty())
    {
        return Error::ErrorCode::INVALID_ARGUMENT;
    }

    DataWriter writer{};
    writer.Write<uint32_t>(Asset::ASSET_CONTAINER_MAGIC);
    writer.Write<uint8_t>(Asset::ASSET_CONTAINER_VERSION);
    writer.Write<uint8_t>(static_cast<uint8_t>(type));
    writer.Write<uint8_t>(typeVersion);
    writer.Write<size_t>(inBuffer.size());

    z_stream zStream{};
    deflateInit2(&zStream, Z_BEST_COMPRESSION, Z_DEFLATED, 15 | 16, 8, Z_DEFAULT_STRATEGY);

    zStream.next_in = inBuffer.data();
    zStream.avail_in = inBuffer.size();
    zStream.data_type = Z_BINARY;

    int ret = Z_OK;
    constexpr size_t chunkSize = 16384;
    std::vector<uint8_t> readBuffer(chunkSize);
    std::vector<uint8_t> compressedData{};

    while (ret == Z_OK)
    {
        zStream.next_out = readBuffer.data();
        zStream.avail_out = readBuffer.size();

        ret = deflate(&zStream, Z_FINISH);

        const ptrdiff_t bytesCompressed = static_cast<ptrdiff_t>(readBuffer.size()) - zStream.avail_out;
        compressedData.insert(compressedData.end(), readBuffer.begin(), readBuffer.begin() + bytesCompressed);
    }

    if (deflateEnd(&zStream) != Z_OK || ret != Z_STREAM_END)
    {
        printf("deflate failed");
        return Error::ErrorCode::COMPRESSION_ERROR;
    }

    writer.Write<size_t>(compressedData.size());
    writer.WriteBuffer<uint8_t>(compressedData);
    writer.CopyToVector(outBuffer);

    return Error::ErrorCode::OK;
}

Error::ErrorCode AssetReader::SaveToFile(const char *filePath,
                                         std::vector<uint8_t> &data,
                                         const Asset::AssetType type,
                                         const uint8_t typeVersion)
{
    FILE *file = fopen(filePath, "wb");
    if (file == nullptr)
    {
        printf("Unable to open file for writing");
        return Error::ErrorCode::CANT_OPEN_FILE;
    }
    std::vector<uint8_t> compressedData;
    const Error::ErrorCode e = Compress(data, compressedData, type, typeVersion);
    fwrite(compressedData.data(), 1, compressedData.size(), file);
    fclose(file);
    return e;
}

Error::ErrorCode AssetReader::LoadFromFile(const char *filePath, Asset &outAsset)
{
    std::FILE *file = std::fopen(filePath, "rb");
    if (file == nullptr)
    {
        return Error::ErrorCode::FILE_NOT_FOUND;
    }
    fseek(file, 0, SEEK_END);
    const size_t dataSize = ftell(file);
    if (dataSize < sizeof(uint32_t) * 4)
    {
        return Error::ErrorCode::INVALID_HEADER;
    }
    std::vector<uint8_t> compressedData(dataSize);
    fseek(file, 0, SEEK_SET);
    fread(compressedData.data(), 1, dataSize, file);
    fclose(file);
    return Decompress(compressedData, outAsset);
}
