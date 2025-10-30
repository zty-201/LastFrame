/**
 * @file MeshCompiler.cpp
 * @brief Implementation of the MeshCompiler class for asset compilation.
 *
 * This file provides the implementation of the `MeshCompiler` class, which
 * determines if a mesh asset needs compilation and performs the compilation
 * process. It includes platform-specific logic for handling file operations.
 *
 * @author Zhang TongYan
 * @date October, 2025
 */
#include "pch.hpp"
#include "MeshCompiler.h"
#include "Mesh.h"
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

            //Precompile data into json and write to output file.
            Mesh::MeshData data{};
            nlohmann::json precompiled_json_data = data.PrecompileFile(src.string().c_str());

            std::ofstream out_stream{ out };
            out_stream << precompiled_json_data.dump();

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