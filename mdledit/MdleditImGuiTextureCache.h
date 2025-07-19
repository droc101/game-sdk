//
// Created by droc101 on 7/18/25.
//

#ifndef MDLEDITIMGUITEXTURECACHE_H
#define MDLEDITIMGUITEXTURECACHE_H
#include "ImGuiTextureAssetCache.h"
#include "ModelRenderer.h"


class MdleditImGuiTextureCache final: public ImGuiTextureAssetCache
{
    public:
        ImTextureID GetTextureID(const std::string &relPath) override
        {
            return ModelRenderer::GetTextureID(relPath);
        }

        ImVec2 GetTextureSize(const std::string &relPath) override
        {
            return ModelRenderer::GetTextureSize(relPath);
        }
};


#endif //MDLEDITIMGUITEXTURECACHE_H
