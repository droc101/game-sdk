//
// Created by droc101 on 7/18/25.
//

#include <libassets/util/Material.h>

Material::Material(DataReader &reader)
{
    const size_t textureStringLength = reader.Read<size_t>();
    reader.ReadString(texture, textureStringLength);
    color = Color(reader, true);
    shader = static_cast<MaterialShader>(reader.Read<uint32_t>());
}

Material::Material(std::string texture, const uint32_t color, const MaterialShader shader):
    texture(std::move(texture)),
    color(Color(color)),
    shader(shader) {}

void Material::Write(DataWriter &writer) const
{
    const size_t strLength = texture.length() + 1;
    writer.Write<size_t>(strLength);
    writer.WriteBuffer<const char>(texture.c_str(), strLength);
    color.WriteFloats(writer);
    writer.Write<uint32_t>(static_cast<uint32_t>(shader));
}

