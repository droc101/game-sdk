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
