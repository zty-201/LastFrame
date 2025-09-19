/**
 * @file    AssetManager.cpp
 * @author  Mohamed Arshad Bin Mohamed Hussain (mohamedarshad.b)
 * @brief   As of 14 September 2025, implement Path-only Asset Manager (no file parsing).
 *
 * @detail  This file contains only path / filesystem operations. No file I/O for parsing/decoding
 *          assets is performed here.
*/
#ifndef DISABLE_ASSET_INPUT
#include "Assets/AssetManager.hpp"
#include <system_error>
#include <fstream>

namespace EngineFramework {

    // -------- small helpers --------
    /**
     * @brief Safely canonicalize a path (ignores errors).
     * @param p Input path.
     * @return Canonical/normalized path; may equal @p p if normalization failed.
     */
    static std::filesystem::path CanonicalSafe(const std::filesystem::path& p) {
        std::error_code ec;
        return std::filesystem::weakly_canonical(p, ec);
    }

    /**
     * @brief Check if path @p a starts with @p prefix (element-wise).
     * @details Used to ensure a destination stays inside the root directory.
     */
    static bool StartsWithPath(const std::filesystem::path& a, const std::filesystem::path& prefix) {
        auto ca = CanonicalSafe(a);
        auto cp = CanonicalSafe(prefix);
        auto ait = ca.begin(), aend = ca.end();
        auto pit = cp.begin(), pend = cp.end();
        for (; pit != pend && ait != aend; ++pit, ++ait) if (*pit != *ait) return false;
        return pit == pend; // every element of prefix matched
    }

    /**
     * @brief Lowercase utility for extensions and case-insensitive compares.
     */
    std::string AssetManagerSystem::toLower(std::string s) {
        for (auto& c : s) c = static_cast<char>(::tolower(static_cast<unsigned char>(c)));
        return s;
    }

    // -------- lifecycle --------
    AssetManagerSystem::AssetManagerSystem() { s_instance = this; }
    AssetManagerSystem& AssetManagerSystem::GetInstance() { return *s_instance; }

    void AssetManagerSystem::SetRoot(const std::filesystem::path& root) {
        m_root = root;
        std::error_code ec;
        std::filesystem::create_directories(m_root, ec); // best-effort
    }

    std::filesystem::path AssetManagerSystem::full(const std::filesystem::path& rel) const {
        return rel.is_absolute() ? rel : (m_root / rel).lexically_normal();
    }
    bool AssetManagerSystem::insideRoot(const std::filesystem::path& abs) const {
        return StartsWithPath(abs, m_root);
    }

    // -------- registration --------
    AssetID AssetManagerSystem::Register(const std::string& name,
        const std::filesystem::path& relativePath,
        bool verifyExists)
    {
        Entry e;
        e.id = m_next++;
        e.name = name;
        e.relative = relativePath.lexically_normal();

        if (verifyExists) {
            std::error_code ec;
            if (!std::filesystem::exists(full(e.relative), ec)) {
                // still register, but mark lastWrite default (not required)
            }
            else {
                e.lastWrite = std::filesystem::last_write_time(full(e.relative), ec);
            }
        }

        m_byName[name] = e;
        m_byId[e.id] = name;

        return e.id;
    }

    void AssetManagerSystem::Unregister(const std::string& name) {
        auto it = m_byName.find(name);
        if (it == m_byName.end()) return;
        AssetID id = it->second.id;
        m_byName.erase(it);
        m_byId.erase(id);
    }

    bool AssetManagerSystem::Has(const std::string& name) const {
        return m_byName.find(name) != m_byName.end();
    }

    AssetID AssetManagerSystem::FindId(const std::string& name) const {
        auto it = m_byName.find(name);
        return (it == m_byName.end()) ? 0 : it->second.id;
    }

    // -------- queries --------
    std::filesystem::path AssetManagerSystem::GetRelativePath(AssetID id) const {
        auto idIt = m_byId.find(id);
        if (idIt == m_byId.end()) return {};

        auto nmIt = m_byName.find(idIt->second);
        if (nmIt == m_byName.end()) return{};

        return nmIt->second.relative;
    }
    std::filesystem::path AssetManagerSystem::GetAbsolutePath(AssetID id) const {
        auto idIt = m_byId.find(id);
        if (idIt == m_byId.end()) return{};

        auto nmIt = m_byName.find(idIt->second);
        if (nmIt == m_byName.end()) return{};

        return full(nmIt->second.relative);
    }
    bool AssetManagerSystem::ExistsOnDisk(AssetID id) const {
        auto idIt = m_byId.find(id);
        if (idIt == m_byId.end()) return false;

        auto nmIt = m_byName.find(idIt->second);
        if (nmIt == m_byName.end()) return false;

        std::error_code ec;
        return std::filesystem::exists(full(nmIt->second.relative), ec);
    }
    bool AssetManagerSystem::ExistsOnDiskByName(const std::string& name) const {
        auto it = m_byName.find(name);
        if (it == m_byName.end()) return false;

        std::error_code ec;
        return std::filesystem::exists(full(it->second.relative), ec);
    }

    // -------- relocation / rename --------
    bool AssetManagerSystem::RelocateByName(const std::string& name,
                                            const std::filesystem::path& newRelativePath,
                                            MovePolicy policy,
                                            bool allowOverwrite)
    {
        auto it = m_byName.find(name);
        if (it == m_byName.end()) return false;

        Entry& e = it->second;

        const auto oldAbs = full(e.relative);
        const auto newRel = newRelativePath.lexically_normal();
        const auto newAbs = full(newRel);

        // keep everything inside root
        if (!insideRoot(newAbs)) return false;

        std::error_code ec;

        if (policy != MovePolicy::UpdateRecordOnly) {
            // make sure destination dir exists
            std::filesystem::create_directories(newAbs.parent_path(), ec);

            const bool destExists = std::filesystem::exists(newAbs, ec);
            if (destExists && !allowOverwrite) return false;

            if (policy == MovePolicy::MoveOnDisk) {
                // try fast rename; if cross-device, fall back to copy+remove
                std::filesystem::rename(oldAbs, newAbs, ec);
                if (ec) {
                    ec.clear();
                    if (!std::filesystem::copy_file(
                        oldAbs, newAbs,
                        allowOverwrite ? std::filesystem::copy_options::overwrite_existing
                        : std::filesystem::copy_options::none,
                        ec))
                        return false;
                    std::filesystem::remove(oldAbs, ec);
                }
            }
            else { // CopyOnDisk
                if (!std::filesystem::copy_file(
                    oldAbs, newAbs,
                    allowOverwrite ? std::filesystem::copy_options::overwrite_existing
                    : std::filesystem::copy_options::none,
                    ec))
                    return false;
            }
        }

        // update the record
        e.relative = newRel;
        e.lastWrite = std::filesystem::last_write_time(newAbs, ec);
        return true;
    }


    bool AssetManagerSystem::RenameName(const std::string& oldName, const std::string& newName) {
        if (m_byName.find(newName) != m_byName.end()) return false; // avoid collision
        auto it = m_byName.find(oldName);
        if (it == m_byName.end()) return false;

        Entry e = it->second;
        m_byName.erase(it);
        e.name = newName;
        m_byName[newName] = e;
        m_byId[e.id] = newName;
        return true;
    }

    // -------- discovery --------
    std::size_t AssetManagerSystem::ImportFolder(const std::filesystem::path& relativeDir,
        const std::vector<std::string>& extensions,
        bool recursive)
    {
        std::size_t count = 0;
        std::vector<std::string> extsLower;
        extsLower.reserve(extensions.size());
        for (auto s : extensions) extsLower.push_back(toLower(std::move(s)));

        auto mode = recursive ? std::filesystem::directory_options::follow_directory_symlink
            : std::filesystem::directory_options::none;

        std::error_code ec;
        const auto baseAbs = full(relativeDir);
        if (!std::filesystem::exists(baseAbs, ec)) return 0;

        auto matchExt = [&](const std::filesystem::path& p) {
            auto ext = toLower(p.extension().string());
            for (auto& e : extsLower) if (ext == e) return true;
            return extsLower.empty(); // if you pass {}, accept all
            };

        if (recursive) {
            for (auto& de : std::filesystem::recursive_directory_iterator(baseAbs, mode, ec)) {
                if (!de.is_regular_file(ec)) continue;
                const auto abs = de.path();
                if (!matchExt(abs)) continue;
                auto rel = std::filesystem::relative(abs, m_root, ec).lexically_normal();
                auto name = abs.stem().string(); // basic name; collisions possible
                if (!Has(name)) { Register(name, rel, /*verify=*/false); ++count; }
            }
        }
        else {
            for (auto& de : std::filesystem::directory_iterator(baseAbs, ec)) {
                if (!de.is_regular_file(ec)) continue;
                const auto abs = de.path();
                if (!matchExt(abs)) continue;
                auto rel = std::filesystem::relative(abs, m_root, ec).lexically_normal();
                auto name = abs.stem().string();
                if (!Has(name)) { Register(name, rel, /*verify=*/false); ++count; }
            }
        }
        return count;
    }

    std::vector<std::pair<std::string, std::filesystem::path>>
        AssetManagerSystem::List() const {
        std::vector<std::pair<std::string, std::filesystem::path>> out;
        out.reserve(m_byName.size());
        for (auto& [n, e] : m_byName) out.emplace_back(n, e.relative);
        return out;
    }

} // namespace EngineFramework

#endif