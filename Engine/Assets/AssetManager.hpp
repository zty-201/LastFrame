/**
 * @file    AssetManager.hpp
 * @author  Mohamed Arshad Bin Mohamed Hussain (mohamedarshad.b)
 * @brief   As of 14 September 2025, Path-only Asset Manager (no file parsing).
 *
 * This manager keeps a database of asset name to relative path under a
 * chosen root folder. It does NOT open files (probably including stb_image in the future).
 * It helps you:
 *  - register assets,
 *  - check if files exist on disk,
 *  - get relative/absolute paths,
 *  - move/copy files inside the root (or only update the record),
 *  - rename asset,
 *  - import a whole folder by extension,
 *  - list all assets.
 *
*/
#ifndef DISABLE_ASSET_INPUT
#pragma once
#ifndef ASSET_MANAGER_HPP
#define ASSET_MANAGER_HPP

#include <string>
#include <unordered_map>
#include <memory>
#include <filesystem>
#include <vector>
#include <cstdint>
#include "ISystem.hpp"

namespace EngineFramework {

    using AssetID = std::uint32_t;

    /**
     * @brief Main path-only asset manager system.
     * @details Works as an engine system (ISystem) but its update hooks are no-ops.
     */
    class AssetManagerSystem : public ISystem {
    public:
        // ---- lifecycle ----
        /**
         * @brief Construct and set the global singleton pointer.
         * @post  GetInstance() returns this object.
         */
        AssetManagerSystem();
        static AssetManagerSystem& GetInstance();
        void Initialize() override {}
        void Update(float) override {}
        void PostUpdate(float) override {}

        // ---- configuration ----
        /**
         * @brief Set the root directory under which all asset paths live.
         * @param root Example: "Extern" or "Assets".
         * @note The directory is created if it does not exist.
         */
        void SetRoot(const std::filesystem::path& root);

        // @brief Get the current root directory.
        [[nodiscard]] const std::filesystem::path& GetRoot() const { return m_root; }

        // ---- registration ----
        /**
         * @brief Register a new asset by name and relative path.
         * @param name          Unique key you will use in code/editor.
         * @param relativePath  Path relative to the root directory.
         * @param verifyExists  If true, checks the file exists (non-blocking).
         * @return A new @ref AssetID handle.
         * @note Registration succeeds even if the file is missing (useful before import).
         */
        AssetID Register(const std::string& name,
                        const std::filesystem::path& relativePath,
                        bool verifyExists = true);

        /**
         * @brief Remove an asset record by name (does not delete the file).
         */
        void    Unregister(const std::string& name);

        /**
         * @brief Check whether an asset name is already registered.
         */
        bool    Has(const std::string& name) const;

        /**
         * @brief Find the numeric id for a name.
         * @return The id or 0 if not found.
         */
        [[nodiscard]] AssetID FindId(const std::string& name) const;

        // ---- path queries ----
        /**
         * @brief Get the stored relative path for an id.
         * @return Empty path if id is unknown.
         */
        [[nodiscard]] std::filesystem::path GetRelativePath(AssetID id) const;

        /**
         * @brief Get the absolute path on disk for an id.
         * @return Empty path if id is unknown.
         */
        [[nodiscard]] std::filesystem::path GetAbsolutePath(AssetID id) const;

        /**
         * @brief Check if the file currently exists on disk (by id).
         */
        [[nodiscard]] bool ExistsOnDisk(AssetID id) const;

        /**
         * @brief Check if the file currently exists on disk (by name).
         */
        [[nodiscard]] bool ExistsOnDiskByName(const std::string& name) const;

        // ---- relocation / renaming ----
        /**
         * @brief How to handle the file when changing the recorded path.
         */
        enum class MovePolicy {
            UpdateRecordOnly,   // Only change the stored path; do not touch the file.
            MoveOnDisk,         // Physically move/rename the file inside the root.
            CopyOnDisk          // Copy the file to the new location inside the root.
        };

        /**
         * @brief Change the path of an asset.
         * @param name            Asset name to modify.
         * @param newRelativePath New path (relative to root).
         * @param policy          Choose whether to move/copy the file or only update the record.
         * @param allowOverwrite  If true, overwrites an existing destination file.
         * @return True on success.
         * @warning The destination must stay inside the root folder.
         */
        bool RelocateByName(const std::string& name,
                            const std::filesystem::path& newRelativePath,
                            MovePolicy policy = MovePolicy::MoveOnDisk,
                            bool allowOverwrite = false);

        /**
         * @brief Rename the asset's *name* (does not change the file).
         * @return False if the new name already exists.
         */
        bool RenameName(const std::string& oldName, const std::string& newName);

        // ---- discovery ----
        /**
         * @brief Import files from a folder by extension.
         * @param relativeDir Directory under root to scan.
         * @param extensions  Extensions to accept (e.g., {".png",".obj"}). Empty = all files.
         * @param recursive   If true, scans subfolders.
         * @return Number of new assets registered.
         */
        std::size_t ImportFolder(const std::filesystem::path& relativeDir,
                                const std::vector<std::string>& extensions,
                                bool recursive = true);

        /**
         * @brief Get a vector of all (name, relativePath) pairs.
         */
        [[nodiscard]] std::vector<std::pair<std::string, std::filesystem::path>> List() const;

    private:
        struct Entry {
            AssetID id = 0;                                 // Numeric handle
            std::string name;                               // Asset name key
            std::filesystem::path relative;                 // Path relative to root
            std::filesystem::file_time_type lastWrite{};    // Last write (optional)
        };

        // helpers
        // Build an absolute path from the stored relative path.
        [[nodiscard]] std::filesystem::path full(const std::filesystem::path& rel) const;

        // Check that an absolute path stays inside the root.
        [[nodiscard]] bool insideRoot(const std::filesystem::path& abs) const;

        // Convert string to lowercase (helper for extensions).
        [[nodiscard]] static std::string toLower(std::string s);

        // data
        std::filesystem::path m_root{ "." };                // Root directory.
        AssetID m_next{ 1 };                                // Next id to hand out.
        std::unordered_map<std::string, Entry> m_byName;    // name to entry
        std::unordered_map<AssetID, std::string>    m_byId; // id to name

        static inline AssetManagerSystem* s_instance = nullptr; // singleton pointer
    };

} // namespace EngineFramework
#endif // ASSET_MANAGER_HPP
#endif