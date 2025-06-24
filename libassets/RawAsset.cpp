//
// Created by droc101 on 6/23/25.
//

#include "include/libassets/RawAsset.h"

uint8_t *RawAsset::GetData() const
{
    return data;
}

std::size_t RawAsset::GetDataSize() const
{
    return data_size;
}

void RawAsset::SetDataSize(std::size_t size)
{
    data_size = size;
}

void RawAsset::FinishLoading()
{
    std::size_t decompressed_size = 0;
    data = Decompress(tempCompressedData, &decompressed_size);
    delete[] tempCompressedData;
}

uint8_t *RawAsset::SaveToBuffer(std::size_t *outSize)
{
    *outSize = data_size;
    return data;
}

