#ifndef ENGINEFRAMEWORK_VIDEO_CLIP_H
#define ENGINEFRAMEWORK_VIDEO_CLIP_H

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <glm/vec2.hpp>

#ifndef DISABLE_FMOD
namespace FMOD
{
    class Sound;
    class Channel;
}
#endif

namespace EngineFramework
{
    enum class AudioGroup : uint8_t;

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

        bool HasAudio() const { return m_hasAudio; }
        void EnsureAudioPlayback(AudioGroup group, float volume);
        void StopAudioPlayback();

    private:
        void Initialize(const std::string& absolutePath);
        void InitializeAudio(const std::wstring& absolutePath);
        bool DecodeNextSample();
        bool RestartStream();
        void UploadFrame(const std::vector<uint8_t>& frameData);
        bool DecodeAudioStream();
        void ReleaseAudioResources();
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
        std::vector<uint8_t> m_rgbaBuffer;

        bool m_hasAudio = false;
        uint32_t m_audioChannels = 0;
        uint32_t m_audioSampleRate = 0;
        uint32_t m_audioBitsPerSample = 0;
        std::vector<uint8_t> m_audioPCM;

#ifndef DISABLE_FMOD
        FMOD::Sound* m_audioSound = nullptr;
        FMOD::Channel* m_audioChannel = nullptr;
#endif
    };
}

#endif // ENGINEFRAMEWORK_VIDEO_CLIP_H
