//
// Created by droc101 on 6/23/25.
//

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <libassets/asset/Asset.h>
#include <libassets/util/AssetContainer.h>
#include <libassets/util/DataReader.h>
#include <libassets/util/DataWriter.h>
#include <libassets/util/Error.h>
#include <libassets/util/FileIo.h>
#include <libassets/util/Logger.h>
#include <string>
#include <vector>
#include <zconf.h>
#include <zlib.h>

Error::ErrorCode AssetContainer::Decompress(std::vector<uint8_t> &asset, AssetContainer &outAsset)
{
    if (!outAsset.reader.bytes.empty())
    {
        return Error::ErrorCode::INVALID_ARGUMENT;
    }
    outAsset.reader.offset = 0;
    if (ASSET_HEADER_SIZE > asset.size())
    {
        return Error::ErrorCode::INVALID_HEADER;
    }

    DataReader reader = DataReader(asset);
    const uint32_t magic = reader.Read<uint32_t>();
    if (magic != ASSET_CONTAINER_MAGIC)
    {
        return Error::ErrorCode::INVALID_HEADER;
    }
    const uint8_t version = reader.Read<uint8_t>();
    if (version != ASSET_CONTAINER_VERSION)
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

    zStream.next_in = asset.data() + ASSET_HEADER_SIZE;
    zStream.avail_in = compressedSize;
    zStream.next_out = outAsset.reader.bytes.data();
    zStream.avail_out = outAsset.reader.size;
    zStream.data_type = Z_BINARY;

    if (inflateInit2(&zStream, MAX_WBITS | 16) != Z_OK)
    {
        Logger::Error("inflateInit2() failed with error: {}", zStream.msg == nullptr ? "(null)" : zStream.msg);
        return Error::ErrorCode::COMPRESSION_ERROR;
    }

    int inflateReturnValue = inflate(&zStream, Z_NO_FLUSH);
    while (inflateReturnValue != Z_STREAM_END)
    {
        if (inflateReturnValue != Z_OK)
        {
            Logger::Error("inflate() failed with error: {}", zStream.msg == nullptr ? "(null)" : zStream.msg);
            return Error::ErrorCode::COMPRESSION_ERROR;
        }
        inflateReturnValue = inflate(&zStream, Z_NO_FLUSH);
    }

    if (inflateEnd(&zStream) != Z_OK)
    {
        Logger::Error("inflateEnd() failed with error: {}", zStream.msg == nullptr ? "(null)" : zStream.msg);
        return Error::ErrorCode::COMPRESSION_ERROR;
    }

    if (outAsset.reader.size != zStream.total_out)
    {
        return Error::ErrorCode::INVALID_BODY;
    }
    return Error::ErrorCode::OK;
}

Error::ErrorCode AssetContainer::Compress(std::vector<uint8_t> &inBuffer,
                                          std::vector<uint8_t> &outBuffer,
                                          const Asset::AssetType type,
                                          const uint8_t typeVersion,
                                          const uint8_t compressionLevel)
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
    writer.Write<uint32_t>(ASSET_CONTAINER_MAGIC);
    writer.Write<uint8_t>(ASSET_CONTAINER_VERSION);
    writer.Write<uint8_t>(static_cast<uint8_t>(type));
    writer.Write<uint8_t>(typeVersion);
    writer.Write<size_t>(inBuffer.size());

    z_stream zStream{};
    deflateInit2(&zStream, compressionLevel, Z_DEFLATED, 15 | 16, 8, Z_DEFAULT_STRATEGY);

    zStream.next_in = inBuffer.data();
    zStream.avail_in = inBuffer.size();
    zStream.data_type = Z_BINARY;

    int ret = Z_OK;
    constexpr size_t CHUNK_SIZE = 16384;
    std::vector<uint8_t> readBuffer(CHUNK_SIZE);
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
        Logger::Error("deflateEnd() failed: {}", zStream.msg);
        return Error::ErrorCode::COMPRESSION_ERROR;
    }

    writer.Write<size_t>(compressedData.size());
    writer.WriteBuffer<uint8_t>(compressedData);
    writer.CopyToVector(outBuffer);

    return Error::ErrorCode::OK;
}

Error::ErrorCode AssetContainer::SaveToFile(const std::string &filePath,
                                            std::vector<uint8_t> &data,
                                            const Asset::AssetType type,
                                            const uint8_t typeVersion,
                                            const uint8_t compressionLevel)
{
    std::vector<uint8_t> compressedData;
    const Error::ErrorCode compressError = Compress(data, compressedData, type, typeVersion, compressionLevel);
    if (compressError != Error::ErrorCode::OK)
    {
        return compressError;
    }
    const Error::ErrorCode writeError = FileIo::WriteBufferToFile(filePath, compressedData);
    return writeError;
}

Error::ErrorCode AssetContainer::LoadFromFile(const std::string &filePath, AssetContainer &outAsset)
{
    std::vector<uint8_t> compressedData;
    const Error::ErrorCode readError = FileIo::ReadFileToBuffer(filePath, compressedData);
    if (readError != Error::ErrorCode::OK)
    {
        return readError;
    }
    return Decompress(compressedData, outAsset);
}
