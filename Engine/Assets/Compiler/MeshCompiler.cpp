#include "pch.hpp"
#include "MeshCompiler.h"

namespace EngineFramework {

    bool MeshCompiler::NeedsCompilation(
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

    bool MeshCompiler::Compile(
        const std::filesystem::path& src,
        const std::filesystem::path& out) const
    {
#ifdef PLATFORM_WINDOWS
        try {
            LOGI("Compiling mesh: %s", src.string().c_str());

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