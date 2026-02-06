#ifndef ASSIMP_WRAPPER_H
#define ASSIMP_WRAPPER_H

#include <assimp/IOStream.hpp>
#include <assimp/IOSystem.hpp>
#include <mutex>
// ========================================
// Custom Assimp IO System for Android
// ========================================

class MemoryIOStream : public Assimp::IOStream {
public:
    MemoryIOStream(const std::vector<unsigned char>& data) : m_data(data), m_pos(0) {}

    size_t Read(void* pvBuffer, size_t pSize, size_t pCount) override {
        size_t bytes = pSize * pCount;
        size_t available = m_data.size() - m_pos;
        size_t toRead = std::min(bytes, available);

        if (toRead > 0) {
            std::memcpy(pvBuffer, m_data.data() + m_pos, toRead);
            m_pos += toRead;
        }

        return toRead / pSize;
    }

    size_t Write(const void*, size_t, size_t) override { return 0; }

    aiReturn Seek(size_t pOffset, aiOrigin pOrigin) override {
        if (pOrigin == aiOrigin_SET) m_pos = pOffset;
        else if (pOrigin == aiOrigin_CUR) m_pos += pOffset;
        else if (pOrigin == aiOrigin_END) m_pos = m_data.size() + pOffset;
        return aiReturn_SUCCESS;
    }

    size_t Tell() const override { return m_pos; }
    size_t FileSize() const override { return m_data.size(); }
    void Flush() override {}

private:
    const std::vector<unsigned char>& m_data;
    size_t m_pos;
};

class AssetIOSystem : public Assimp::IOSystem {
public:
    AssetIOSystem(const std::string& basePath) : m_basePath(basePath) {}

    bool Exists(const char* pFile) const override {
        std::string fullPath = resolvePath(pFile);
        try {
            auto data = EngineFramework::AssetManagerSystem::GetDataFromFile(fullPath);
            return !data.empty();
        }
        catch (...) {
            return false;
        }
    }

    char getOsSeparator() const override { return '/'; }

    Assimp::IOStream* Open(const char* pFile, const char* pMode = "rb") override {
		UNREFERENCED_PARAMETER(pMode);
        std::string fullPath = resolvePath(pFile);

        try {
            // Load file into memory
            auto data = std::make_shared<std::vector<unsigned char>>(
                EngineFramework::AssetManagerSystem::GetDataFromFile(fullPath)
            );

            // Store it so it stays alive
            static std::mutex file_cache_mutex;
            {
                std::lock_guard<std::mutex> file_cache_lock(file_cache_mutex);
                m_fileCache[fullPath] = data;
            }

            // Return a stream to it
            return new MemoryIOStream(*data);
        }
        catch (const std::exception& e) {
            LOGE("Failed to open asset: %s (error: %s)", fullPath.c_str(), e.what());
            return nullptr;
        }
    }

    void Close(Assimp::IOStream* pFile) override {
        delete pFile;
    }

private:
    std::string resolvePath(const char* pFile) const {
        std::string file(pFile);

        // If the file path is absolute or already starts with the base directory, use it as-is
        if (file.find("../../Extern") == 0 || file.find("Extern") == 0) {
            // Already a full path, normalize slashes and return
            std::replace(file.begin(), file.end(), '\\', '/');
            return file;
        }

        // Extract just the filename from pFile (no directory)
        size_t lastSlash = file.find_last_of("/\\");
        std::string filename = (lastSlash != std::string::npos)
            ? file.substr(lastSlash + 1)
            : file;

        // Extract the directory from m_basePath
        lastSlash = m_basePath.find_last_of("/\\");
        std::string baseDir = (lastSlash != std::string::npos)
            ? m_basePath.substr(0, lastSlash + 1)
            : "";

        // Combine: baseDir + filename only
        std::string result = baseDir + filename;

        // Normalize slashes (forward slashes work on all platforms)
        std::replace(result.begin(), result.end(), '\\', '/');

        LOGI("Resolved path: '%s' -> '%s'", pFile, result.c_str());
        return result;
    }

    std::string m_basePath;
    mutable std::unordered_map<std::string, std::shared_ptr<std::vector<unsigned char>>> m_fileCache;
};

#endif