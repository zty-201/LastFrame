/**
 * @file TextureCompiler.h
 * @brief Declaration of the TextureCompiler class for texture asset compilation.
 *
 * This file defines the `TextureCompiler` class, which implements the
 * `IAssetCompiler` interface. It provides functionality to determine if a
 * texture asset needs compilation, perform the compilation, and specify the
 * source and output file extensions for texture assets.
 *
 * @author Zhang TongYan
 * @date October, 2025
 */
#ifndef TEXTURE_COMPILER_H
#define TEXTURE_COMPILER_H

#include "AssetCompiler.h"

namespace EngineFramework {

    class TextureCompiler : public IAssetCompiler
    {
    public:
        bool NeedsCompilation(const std::filesystem::path& src,
            const std::filesystem::path& out) const override;
        bool Compile(const std::filesystem::path& src,
            const std::filesystem::path& out) const override;
        std::string GetSourceExt() const override { return ".png"; }
        std::string GetOutputExt() const override { return ".compiled"; }
    };

} // namespace EngineFramework

#endif