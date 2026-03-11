#ifndef ENGINEFRAMEWORK_VIDEO_CLIP_H
#define ENGINEFRAMEWORK_VIDEO_CLIP_H

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <glm/vec2.hpp>

namespace EngineFramework
{
    class VideoClip
    {
    public:
        explicit VideoClip(const std::string& absolutePath, bool loop = true);
        ~VideoClip();

        VideoClip(const VideoClip&) = delete;
        VideoClip& operator=(const VideoClip&) = delete;

        bool IsReady() const;
        unsigned int GetTextureId() const { return m_textureId; }
        glm::ivec2 GetDimensions() const { return glm::ivec2{ static_cast<int>(m_width), static_cast<int>(m_height) }; }

        void Update(double deltaSeconds);
        void Restart();

    private:
        void Initialize(const std::string& absolutePath);
        bool DecodeNextSample();
        bool RestartStream();
        void UploadFrame(const std::vector<uint8_t>& frameData);
        static void EnsureMediaFoundation();
        static std::wstring ToWide(const std::string& path);

        struct DecoderState;
        std::unique_ptr<DecoderState> m_decoder;
        bool m_initialized = false;
        bool m_loop = true;
        bool m_reachedEnd = false;
        double m_secondsPerFrame = 1.0 / 30.0;
        double m_timeAccumulator = 0.0;
        uint32_t m_width = 0;
        uint32_t m_height = 0;
        unsigned int m_textureId = 0;
        std::vector<uint8_t> m_frameBuffer;
    };
}

#endif // ENGINEFRAMEWORK_VIDEO_CLIP_H
