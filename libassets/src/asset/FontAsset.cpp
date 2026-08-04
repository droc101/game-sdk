//
// Created by droc101 on 7/23/25.
//

#include <cstddef>
#include <cstdint>
#include <format>
#include <libassets/asset/Asset.h>
#include <libassets/asset/FontAsset.h>
#include <libassets/util/DataReader.h>
#include <libassets/util/DataWriter.h>
#include <libassets/util/Error.h>
#include <string>
#include <vector>

Asset::AssetType FontAsset::GetAssetType() const
{
    return AssetType::ASSET_TYPE_FONT;
}

uint8_t FontAsset::GetAssetTypeVersion() const
{
    return FONT_ASSET_VERSION;
}

Error::ErrorCode FontAsset::LoadFromBuffer(DataReader &reader)
{
    charWidth = reader.Read<uint8_t>();
    textureHeight = reader.Read<uint8_t>();
    baseline = reader.Read<uint8_t>();
    charSpacing = reader.Read<uint8_t>();
    lineSpacing = reader.Read<uint8_t>();
    spaceWidth = reader.Read<uint8_t>();
    defaultSize = reader.Read<uint8_t>();
    uppercaseOnly = reader.Read<bool>();
    const size_t textureLength = reader.Read<size_t>();
    reader.ReadString(texture, textureLength);
    const uint8_t charCount = reader.Read<uint8_t>();
    if (charCount > FONT_MAX_SYMBOLS)
    {
        return Error::ErrorCode::INVALID_BODY;
    }
    for (uint8_t i = 0; i < charCount; i++)
    {
        const char character = reader.Read<char>();
        const uint8_t width = reader.Read<uint8_t>();
        chars.push_back(character);
        charWidths.push_back(width);
    }

    return Error::ErrorCode::OK;
}

Error::ErrorCode FontAsset::SaveToBuffer(DataWriter &writer) const
{
    writer.Write<uint8_t>(charWidth);
    writer.Write<uint8_t>(textureHeight);
    writer.Write<uint8_t>(baseline);
    writer.Write<uint8_t>(charSpacing);
    writer.Write<uint8_t>(lineSpacing);
    writer.Write<uint8_t>(spaceWidth);
    writer.Write<uint8_t>(defaultSize);
    writer.Write<bool>(uppercaseOnly);
    writer.Write<size_t>(texture.length() + 1);
    writer.WriteBuffer(texture.c_str(), texture.length() + 1);
    writer.Write<uint8_t>(chars.size());
    for (size_t i = 0; i < chars.size(); i++)
    {
        writer.Write<char>(chars.at(i));
        writer.Write<uint8_t>(charWidths.at(i));
    }
    return Error::ErrorCode::OK;
}

std::vector<std::string> FontAsset::GetCharListForDisplay()
{
    std::vector<std::string> list{};
    for (size_t i = 0; i < FONT_MAX_SYMBOLS; i++)
    {
        const char c = FONT_VALID_CHARS.at(i);
        list.push_back(std::format("{} (0x{:02X})", c, c));
    }
    return list;
}
