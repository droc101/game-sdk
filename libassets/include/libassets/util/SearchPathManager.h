//
// Created by droc101 on 3/6/26.
//

#pragma once

#include <libassets/asset/DataAsset.h>
#include <string>
#include <vector>

class SearchPathManager
{
    public:
        struct AssetResult
        {
                std::string relativePath;
                std::string absolutePath;
        };

        SearchPathManager() = default;
        SearchPathManager(DataAsset &gameConfig,
                          const std::string &executableFolder,
                          const std::string &configParentFolder);

        [[nodiscard]] static std::vector<std::string> ScanFolder(const std::string &directoryPath,
                                                                 const std::string &extension,
                                                                 bool isRoot);

        [[nodiscard]] std::vector<AssetResult> ScanAssetFolder(const std::string &assetFolder,
                                                               const std::string &extension) const;

        [[nodiscard]] std::vector<std::string> ScanAssetFolderA(const std::string &assetFolder,
                                                                const std::string &extension) const;

        [[nodiscard]] std::vector<std::string> ScanAssetFolderR(const std::string &assetFolder,
                                                                const std::string &extension) const;

        [[nodiscard]] std::string GetAssetPath(const std::string &relPath) const;

    private:
        std::vector<std::string> assetPaths{};
};
