#pragma once
#include <string>
#include <filesystem>
#include <vector>

class ScriptAsset {
public:
    ScriptAsset() = default;
    ~ScriptAsset() = default;
    ScriptAsset(const ScriptAsset&) = delete;
    ScriptAsset& operator=(const ScriptAsset&) = delete;
    ScriptAsset(ScriptAsset&&) noexcept = default;
    ScriptAsset& operator=(ScriptAsset&&) noexcept = default;

    std::filesystem::path relPath;  // e.g. "Extern/Scripts/ai/guard.lua"
    std::string code;               // Preloaded Lua source text
};
