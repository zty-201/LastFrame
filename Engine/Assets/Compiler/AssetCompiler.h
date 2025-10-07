/**
 * @file AssetCompiler.h
 * @brief Interface for asset compilation in the EngineFramework.
 *
 * This file defines the `IAssetCompiler` interface, which provides methods
 * for determining if an asset needs compilation, performing the compilation,
 * and retrieving file extensions for source and output assets.
 *
 * @author Zhang TongYan
 * @date October, 2025
 */
#ifndef ASSET_COMPILER_H
#define ASSET_COMPILER_H

namespace EngineFramework {

    class IAssetCompiler
    {
    public:
        virtual ~IAssetCompiler() = default;

        virtual bool NeedsCompilation(
            const std::filesystem::path& source,
            const std::filesystem::path& output) const = 0;

        virtual bool Compile(
            const std::filesystem::path& source,
            const std::filesystem::path& output) const = 0;

        virtual std::string GetSourceExt() const = 0;
        virtual std::string GetOutputExt() const = 0;
    };

} // namespace EngineFramework

#endif