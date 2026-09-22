//
// Created by droc101 on 9/21/26.
//

#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <filesystem>
#include <format>
#include <iterator>
#include <libassets/asset/ShaderAsset.h>
#include <libassets/util/ArgumentParser.h>
#include <libassets/util/Error.h>
#include <libassets/util/Logger.h>
#include <libassets/util/SearchPathManager.h>
#include <ranges>
#include <string>
#include <utility>
#include <vector>

static std::vector<std::pair<std::string, ShaderAsset::ShaderType>> files{};

static std::string outputFolder{};
static bool replicateFolderStructure = false;
static std::string sourcesBaseFolder{};
static bool enableOptimization = false;
static bool debugInfo = false;
static bool dumpBinaries = false;

static Error::ErrorCode Compile()
{
    if (!std::filesystem::is_directory(outputFolder))
    {
        return Error::ErrorCode::INVALID_DIRECTORY;
    }

    for (const std::pair<std::string, ShaderAsset::ShaderType> &i: files)
    {
        const std::string &file = i.first;
        const ShaderAsset::ShaderType kind = i.second;
        ShaderAsset shader;
        Error::ErrorCode e = shader.Import(file);
        if (e != Error::ErrorCode::OK)
        {
            return e;
        }
        shader.type = kind;
        std::string suffix;
        switch (shader.type)
        {
            case ShaderAsset::ShaderType::SHADER_KIND_FRAGMENT:
                suffix = "f";
                break;
            case ShaderAsset::ShaderType::SHADER_KIND_VERTEX:
                suffix = "v";
                break;
            case ShaderAsset::ShaderType::SHADER_KIND_COMPUTE:
                suffix = "c";
                break;
            case ShaderAsset::ShaderType::SHADER_KIND_GEOMETRY:
                suffix = "g";
                break;
        }

        const std::string filename = std::filesystem::path(file).stem().string();
        std::string outputPath = std::format("{}/{}_{}.{}",
                                             outputFolder,
                                             filename,
                                             suffix,
                                             ShaderAsset::SHADER_ASSET_EXTENSION);
        if (replicateFolderStructure)
        {
            if (!sourcesBaseFolder.empty() && file.starts_with(sourcesBaseFolder))
            {
                const std::string relativePath = std::filesystem::path(file.substr(sourcesBaseFolder.length()))
                                                         .parent_path()
                                                         .string();
                const std::filesystem::path finalOutputDirectory{outputFolder + "/" + relativePath};
                std::filesystem::create_directories(finalOutputDirectory);
                outputPath = std::format("{}/{}_{}.{}",
                                         finalOutputDirectory.string(),
                                         filename,
                                         suffix,
                                         ShaderAsset::SHADER_ASSET_EXTENSION);
            }
        }

        Logger::Info("Compiling \"{}\"...", file);
        std::string errorLog{};
        e = shader.SaveToAssetEx(outputPath, enableOptimization, debugInfo, &errorLog, file, dumpBinaries);
        if (!errorLog.empty())
        {
            Logger::Info("Log: {}", errorLog);
        }
        if (e != Error::ErrorCode::OK)
        {
            return e;
        }
    }

    return Error::ErrorCode::OK;
}

static void AddSourceFiles(const std::vector<std::string> &paths)
{
    for (const std::string &file: paths)
    {
        bool alreadyAdded = false;
        for (const std::string &existingFile: files | std::views::keys)
        {
            if (existingFile == file)
            {
                alreadyAdded = true;
                break;
            }
        }
        if (alreadyAdded)
        {
            Logger::Warning("Source file \"{}\" was already added.", file);
            continue;
        }

        if (file.ends_with(".inc.glsl"))
        {
            Logger::Warning("Skipping include file \"{}\"...", file);
            continue;
        }

        ShaderAsset::ShaderType kind = ShaderAsset::ShaderType::SHADER_KIND_FRAGMENT;
        if (file.ends_with(".frag") || file.ends_with("_f.glsl"))
        {
            kind = ShaderAsset::ShaderType::SHADER_KIND_FRAGMENT;
        } else if (file.ends_with(".vert") || file.ends_with("_v.glsl"))
        {
            kind = ShaderAsset::ShaderType::SHADER_KIND_VERTEX;
        } else if (file.ends_with(".comp") || file.ends_with("_c.glsl"))
        {
            kind = ShaderAsset::ShaderType::SHADER_KIND_COMPUTE;
        } else if (file.ends_with(".geom") || file.ends_with("_g.glsl"))
        {
            kind = ShaderAsset::ShaderType::SHADER_KIND_GEOMETRY;
        }
        files.emplace_back(std::make_pair(file, kind));
    }
}

static void AddSourcesFolder(const std::string &folder)
{
    const std::vector<std::string> vertexSources = SearchPathManager::ScanFolder(folder, ".vert", true);
    const std::vector<std::string> fragmentSources = SearchPathManager::ScanFolder(folder, ".frag", true);
    const std::vector<std::string> computeSources = SearchPathManager::ScanFolder(folder, ".comp", true);
    const std::vector<std::string> geometrySources = SearchPathManager::ScanFolder(folder, ".geom", true);

    std::vector<std::string> sourcePaths{};
    sourcePaths.reserve(vertexSources.size() + fragmentSources.size() + computeSources.size() + geometrySources.size());
    std::ranges::copy(vertexSources, std::back_inserter(sourcePaths));
    std::ranges::copy(fragmentSources, std::back_inserter(sourcePaths));
    std::ranges::copy(computeSources, std::back_inserter(sourcePaths));
    std::ranges::copy(geometrySources, std::back_inserter(sourcePaths));

    for (std::string &path: sourcePaths)
    {
        if (folder.ends_with('/') || folder.ends_with('\\'))
        {
            path = folder + path;
        } else
        {
            path = folder + '/' + path;
        }
    }

    std::ranges::sort(sourcePaths, [](const std::string &a, const std::string &b) {
        return std::filesystem::path(a).filename().string() < std::filesystem::path(b).filename().string();
    });

    AddSourceFiles(sourcePaths);
}

int main(const int argc, const char **argv)
{
    Logger::Info("GAME SDK Shader Compiler");
    const ArgumentParser args = ArgumentParser(argc, argv);

    if (args.HasFlag("--help") || args.HasFlag("-h"))
    {
        printf("Usage: shdcomp [options]\n");
        printf("--output-directory=/path/to/output/folder/....................The path to the folder where compiled "
               "GSHD files should be written.\n");
        printf("--source-file=/path/to/glsl/source/file.......................The path to the GLSL source file to "
               "compile.\n");
        printf("--source-directory=/path/to/glsl/sources/folder...............The path to a folder containing GLSL "
               "files to compile.\n");
        printf("--base-directory=/path/to/glsl/sources/folder.................Replicates the folder structure starting "
               "at this path.\n");
        printf("-O, --optimize................................................Enable optimization.\n");
        printf("-g, --debug-info..............................................Enable debug info.\n");
        printf("-r, --replicate-source-directory..............................Replicates the folder structure of the "
               "path specified in --source-directory.\n");
        printf("-d, --dump-spirv..............................................Dump raw SPIR-V binaries alongside the "
               "compiled shaders.\n");
        return 0;
    }

    const bool hasOutputDirectory = args.HasFlagWithValue("--output-directory");
    if (hasOutputDirectory)
    {
        outputFolder = args.GetFlagValue("--output-directory");
    }

    replicateFolderStructure = args.HasFlagWithValue("--base-directory");
    if (replicateFolderStructure)
    {
        sourcesBaseFolder = args.GetFlagValue("--base-directory");
    }

    if (args.HasFlagWithValue("--source-file"))
    {
        AddSourceFiles({args.GetFlagValue("--source-file")});
    } else if (args.HasFlagWithValue("--source-directory"))
    {
        const std::string &dir = args.GetFlagValue("--source-directory");
        if (args.HasFlag("-r") || args.HasFlag("--replicate-source-directory"))
        {
            replicateFolderStructure = true;
            sourcesBaseFolder = dir;
        }
        AddSourcesFolder(dir);
    }

    enableOptimization = args.HasFlag("-O") || args.HasFlag("--optimize");
    debugInfo = args.HasFlag("-g") || args.HasFlag("--debug-info");
    dumpBinaries = args.HasFlag("-d") || args.HasFlag("--dump-spirv");

    if (!hasOutputDirectory)
    {
        Logger::Error("--output-directory must be set!");
        return 1;
    }
    if (files.empty())
    {
        Logger::Error("No source files specified! Either --source-file or --source-directory must be set.");
        return 1;
    }

    Logger::Info("Compiling shaders...");
    const Error::ErrorCode e = Compile();
    if (e != Error::ErrorCode::OK)
    {
        Logger::Error("Failed to compile shaders: {}", Error::ErrorString(e));
        return 1;
    }

    Logger::Info("Successfully compiled.");
    return 0;
}
