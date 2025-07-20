//
// Created by droc101 on 7/18/25.
//

#include <libassets/util/Material.h>

Material::Material(DataReader &reader)
{
    reader.ReadString(texture, 64);
    color = Color(reader, false);
    shader = static_cast<MaterialShader>(reader.Read<uint32_t>());
}

Material::Material(std::string texture, const uint32_t color, const MaterialShader shader):
    texture(std::move(texture)),
    color(Color(color)),
    shader(shader) {}

void Material::Write(DataWriter &writer) const
{
    writer.WriteBuffer<const char>(texture.c_str(), 64);
    color.WriteUint32(writer);
    writer.Write<uint32_t>(static_cast<uint32_t>(shader));
}

