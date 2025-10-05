/**
 * @file MeshCompiler.h
 * @brief Declaration of the MeshCompiler class for mesh asset compilation.
 *
 * This file defines the `MeshCompiler` class, which implements the
 * `IAssetCompiler` interface. It provides functionality to determine if a
 * mesh asset needs compilation, perform the compilation, and specify the
 * source and output file extensions for mesh assets.
 *
 * @author Zhang TongYan
 * @date October, 2025
 */
#ifndef MESH_COMPILER_H
#define MESH_COMPILER_H

#include "AssetCompiler.h"

namespace EngineFramework {

    class MeshCompiler : public IAssetCompiler
    {
    public:
        bool NeedsCompilation(const std::filesystem::path& src,
            const std::filesystem::path& out) const override;
        bool Compile(const std::filesystem::path& src,
            const std::filesystem::path& out) const override;
        std::string GetSourceExt() const override { return ".obj"; }
        std::string GetOutputExt() const override { return ".compiled"; }
    };

} // namespace EngineFramework

#endif