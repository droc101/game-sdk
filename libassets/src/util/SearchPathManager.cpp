//
// Created by droc101 on 3/6/26.
//

#include <libassets/type/Param.h>
#include <libassets/util/SearchPathManager.h>
#include <string>

SearchPathManager::SearchPathManager(DataAsset &gameConfig, const std::string &executableFolder)
{
    ParamVector &searchPathData = gameConfig.data["search_paths"].GetRef<ParamVector>({});
    for (Param &p: searchPathData)
    {
        if (p.GetType() == Param::ParamType::PARAM_TYPE_STRING)
        {
            const std::string searchPath = executableFolder + "/" + p.Get<std::string>("engine");
            assetPaths.push_back(searchPath);
        } else if (p.GetType() == Param::ParamType::PARAM_TYPE_KV_LIST)
        {
            KvList &searchPathKvl = p.GetRef<KvList>({});
            const bool isAbsolute = searchPathKvl["path_is_absolute"].Get<bool>(false);
            const std::string path = searchPathKvl["search_path"].Get<std::string>("engine");
            if (isAbsolute)
            {
                assetPaths.push_back(path);
            } else
            {
                assetPaths.push_back(std::format("{}/{}", executableFolder, path));
            }
        }
    }
}

std::string SearchPathManager::GetAssetPath(const std::string &relPath) const
{
    for (const std::string &searchPath: assetPaths)
    {
        const std::string p = std::format("{}/{}", searchPath, relPath);
        if (std::filesystem::exists(std::filesystem::path(p)))
        {
            return p;
        }
    }
    return "";
}

std::vector<std::string> SearchPathManager::ScanFolder(const std::string &directoryPath,
                                                       const std::string &extension,
                                                       const bool isRoot)
{
    std::vector<std::string> files;
    try
    {
        for (const std::filesystem::directory_entry &entry: std::filesystem::directory_iterator(directoryPath))
        {
            if (entry.is_regular_file())
            {
                if (entry.path().extension() == extension)
                {
                    files.push_back(entry.path().string());
                }
            } else if (entry.is_directory())
            {
                const std::vector<std::string> subfolderFiles = ScanFolder(entry.path().string(), extension, false);
                files.insert(files.end(), subfolderFiles.begin(), subfolderFiles.end());
            }
        }
    } catch (const std::filesystem::filesystem_error &exception)
    {
        printf("std::filesystem_error: %s\n", exception.what());
    }
    if (isRoot)
    {
        for (std::string &file: files)
        {
            file = file.substr(directoryPath.length() + 1);
        }
        std::ranges::sort(files, [](const std::string &a, const std::string &b) {
            return std::filesystem::path(a).filename().string() < std::filesystem::path(b).filename().string();
        });
    }
    return files;
}

std::vector<SearchPathManager::AssetResult> SearchPathManager::ScanAssetFolder(const std::string &assetFolder,
                                                                               const std::string &extension) const
{
    std::vector<std::string> paths{};
    std::vector<std::string> found{};
    for (const std::string &searchPath: assetPaths)
    {
        const std::string absPath = std::format("{}/{}", searchPath, assetFolder);
        const std::vector<std::string> searchPathContents = ScanFolder(absPath, extension, true);
        for (const std::string &content: searchPathContents)
        {
            if (std::ranges::find(found, content) == found.end())
            {
                found.push_back(content);
                paths.push_back(std::format("{}/{}", absPath, content));
            }
        }
    }
    assert(paths.size() == found.size());
    std::vector<AssetResult> results{};
    for (size_t i = 0; i < paths.size(); i++)
    {
        AssetResult res = {
            .relativePath = found.at(i),
            .absolutePath = paths.at(i),
        };
        results.emplace_back(res);
    }
    return results;
}

std::vector<std::string> SearchPathManager::ScanAssetFolderA(const std::string &assetFolder,
                                                             const std::string &extension) const
{
    const std::vector<AssetResult> assets = ScanAssetFolder(assetFolder, extension);
    std::vector<std::string> absPaths{};
    for (const AssetResult &res: assets)
    {
        absPaths.push_back(res.absolutePath);
    }
    return absPaths;
}

std::vector<std::string> SearchPathManager::ScanAssetFolderR(const std::string &assetFolder,
                                                             const std::string &extension) const
{
    const std::vector<AssetResult> assets = ScanAssetFolder(assetFolder, extension);
    std::vector<std::string> relPaths{};
    for (const AssetResult &res: assets)
    {
        relPaths.push_back(res.relativePath);
    }
    return relPaths;
}
