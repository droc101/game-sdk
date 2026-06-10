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
                /// The path in the asset filesystem
                std::string relativePath;
                /// The path on disk
                std::string absolutePath;
        };

        SearchPathManager() = default;
        SearchPathManager(DataAsset &gameConfig,
                          const std::string &executableFolder,
                          const std::string &configParentFolder);

        /**
         * Scan a real folder for all files with a given extension
         * @param directoryPath The path to the on-disk folder to scan
         * @param extension The extension to scan for (e.g. ".txt")
         * @param isRoot Set this to true
         */
        [[nodiscard]] static std::vector<std::string> ScanFolder(const std::string &directoryPath,
                                                                 const std::string &extension,
                                                                 bool isRoot);

        /**
         * Scan an asset folder for all files with a given extension
         * @param assetFolder The asset folder path (e.g. "textures/level")
         * @param extension The extension to scan for (e.g. ".gtex")
         * @return a vector of AssetResult structs containing the absolute (on-disk) and relative (asset filesystem) paths
         */
        [[nodiscard]] std::vector<AssetResult> ScanAssetFolder(const std::string &assetFolder,
                                                               const std::string &extension) const;

        /**
        * Scan an asset folder for all files with a given extension
         * @param assetFolder The asset folder path (e.g. "textures/level")
         * @param extension The extension to scan for (e.g. ".gtex")
         * @return The absolute (on-disk) paths to all results
         */
        [[nodiscard]] std::vector<std::string> ScanAssetFolderA(const std::string &assetFolder,
                                                                const std::string &extension) const;

        /**
        * Scan an asset folder for all files with a given extension
         * @param assetFolder The asset folder path (e.g. "textures/level")
         * @param extension The extension to scan for (e.g. ".gtex")
         * @return The relative (asset filesystem) paths to all results
         */
        [[nodiscard]] std::vector<std::string> ScanAssetFolderR(const std::string &assetFolder,
                                                                const std::string &extension) const;

        /**
         * Get the absolute (on-disk) path of a given relative (asset filesystem) path
         */
        [[nodiscard]] std::string GetAssetPath(const std::string &relPath) const;

    private:
        std::vector<std::string> assetPaths{};
};
