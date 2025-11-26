/**
 * @file AnimationCompiler.h
 * @brief Declaration of the AnimationCompiler class for Animation asset compilation.
 *
 * This file defines the `AnimationCompiler` class, which implements the
 * `IAssetCompiler` interface. It provides functionality to determine if a
 * Animation asset needs compilation, perform the compilation, and specify the
 * source and output file extensions for Animation assets.
 *
 * @author Zhang TongYan
 * @date October, 2025
 */
#ifndef ANIMATION_COMPILER_H
#define ANIMATION_COMPILER_H

#include "AssetCompiler.h"

namespace EngineFramework {

    class AnimationCompiler : public IAssetCompiler
    {
    public:
        bool NeedsCompilation(const std::filesystem::path& src,
            const std::filesystem::path& out) const override;
        bool Compile(const std::filesystem::path& src,
            const std::filesystem::path& out) const override;
        std::string GetSourceExt() const override { return ".fbx"; }
        std::string GetOutputExt() const override { return ".compiled"; }
    };

} // namespace EngineFramework

#endif