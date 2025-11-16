//
// Created by droc101 on 9/5/25.
//

#include <libassets/type/WallMaterial.h>
#include <string>

WallMaterial::WallMaterial(const std::string &material)
{
    this->material = material;
}

WallMaterial::WallMaterial(nlohmann::ordered_json json)
{
    material = json.value("material", "");
    uvOffset = {json["uvOffset"].value("x", 0.0f), json["uvOffset"].value("y", 0.0f)};
    uvScale = {json["uvScale"].value("x", 0.0f), json["uvScale"].value("y", 0.0f)};
}


nlohmann::ordered_json WallMaterial::GenerateJson() const
{
    nlohmann::ordered_json j{};
    j["material"] = material;
    j["uvOffset"]["x"] = uvOffset.at(0);
    j["uvOffset"]["y"] = uvOffset.at(1);
    j["uvScale"]["x"] = uvScale.at(0);
    j["uvScale"]["y"] = uvScale.at(1);
    return j;
}
