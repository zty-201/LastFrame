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