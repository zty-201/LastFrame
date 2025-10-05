#include "pch.hpp"
#include "TextureCompiler.h"
#include "AssetManager.hpp"

namespace EngineFramework {

    bool TextureCompiler::NeedsCompilation(
        const std::filesystem::path& src,
        const std::filesystem::path& out) const
    {
#ifdef PLATFORM_WINDOWS
        if (!std::filesystem::exists(out))
            return true;
        return std::filesystem::last_write_time(src) >
            std::filesystem::last_write_time(out);
#else
        return false;
#endif
    }

    bool TextureCompiler::Compile(
        const std::filesystem::path& src,
        const std::filesystem::path& out) const
    {
#ifdef PLATFORM_WINDOWS
        try {
            LOGI("Compiling texture: %s", src.string().c_str());
			// For simplicity, just copy the file. Real implementation would compress/convert it.
            std::filesystem::create_directories(out.parent_path());
            std::filesystem::copy(src, out,
                std::filesystem::copy_options::overwrite_existing);

            return true;
        }
        catch (std::exception& e) {
            LOGE("Compilation failed: %s", e.what());
            return false;
        }
#else
        return true;
#endif
    }

} // namespace EngineFramework