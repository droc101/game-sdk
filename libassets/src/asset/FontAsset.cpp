//
// Created by droc101 on 7/23/25.
//

#include <cstddef>
#include <cstdint>
#include <format>
#include <libassets/asset/FontAsset.h>
#include <libassets/type/Asset.h>
#include <libassets/util/AssetReader.h>
#include <libassets/util/DataWriter.h>
#include <libassets/util/Error.h>
#include <string>
#include <vector>

Error::ErrorCode FontAsset::CreateFromAsset(const char *assetPath, FontAsset &font)
{
    Asset asset;
    const Error::ErrorCode error = AssetReader::LoadFromFile(assetPath, asset);
    if (error != Error::ErrorCode::OK)
    {
        return error;
    }
    if (asset.type != Asset::AssetType::ASSET_TYPE_FONT)
    {
        return Error::ErrorCode::INCORRECT_FORMAT;
    }
    if (asset.typeVersion != FONT_ASSET_VERSION)
    {
        return Error::ErrorCode::INCORRECT_VERSION;
    }
    font = FontAsset();
    font.charWidth = asset.reader.Read<uint8_t>();
    font.textureHeight = asset.reader.Read<uint8_t>();
    font.baseline = asset.reader.Read<uint8_t>();
    font.charSpacing = asset.reader.Read<uint8_t>();
    font.lineSpacing = asset.reader.Read<uint8_t>();
    font.spaceWidth = asset.reader.Read<uint8_t>();
    font.defaultSize = asset.reader.Read<uint8_t>();
    font.uppercaseOnly = asset.reader.Read<bool>();
    const size_t textureLength = asset.reader.Read<size_t>();
    asset.reader.ReadString(font.texture, textureLength);
    const uint8_t charCount = asset.reader.Read<uint8_t>();
    if (charCount > FONT_MAX_SYMBOLS)
    {
        return Error::ErrorCode::INVALID_BODY;
    }
    for (uint8_t i = 0; i < charCount; i++)
    {
        const char character = asset.reader.Read<char>();
        const uint8_t width = asset.reader.Read<uint8_t>();
        font.chars.push_back(character);
        font.charWidths.push_back(width);
    }

    return Error::ErrorCode::OK;
}

Error::ErrorCode FontAsset::SaveAsAsset(const char *assetPath) const
{
    std::vector<uint8_t> buffer;
    SaveToBuffer(buffer);
    return AssetReader::SaveToFile(assetPath, buffer, Asset::AssetType::ASSET_TYPE_FONT, FONT_ASSET_VERSION);
}


void FontAsset::SaveToBuffer(std::vector<uint8_t> &buffer) const
{
    DataWriter writer{};
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
    writer.CopyToVector(buffer);
}

std::vector<std::string> FontAsset::GetCharListForDisplay()
{
    std::vector<std::string> list{};
    for (size_t i = 0; i < FONT_MAX_SYMBOLS; i++)
    {
        const char c = FONT_VALID_CHARS[i];
        list.push_back(std::format("{} (0x{:02X})", c, c));
    }
    return list;
}
