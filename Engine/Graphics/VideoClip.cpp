#include "pch.hpp"
#include "VideoClip.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <mutex>
#include <stdexcept>

#ifdef PLATFORM_WINDOWS
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <propvarutil.h>
#include <wrl/client.h>
#include <cstdlib>

namespace
{
    void ShutdownMediaFoundation()
    {
        MFShutdown();
    }
}
#endif

namespace EngineFramework
{
    struct VideoClip::DecoderState
    {
#ifdef PLATFORM_WINDOWS
        Microsoft::WRL::ComPtr<IMFSourceReader> reader;
#endif
    };

    VideoClip::VideoClip(const std::string& absolutePath, bool loop)
        : m_loop(loop)
        , m_decoder(std::make_unique<DecoderState>())
    {
        Initialize(absolutePath);
    }

    VideoClip::~VideoClip()
    {
#ifdef PLATFORM_WINDOWS
        if (m_decoder)
        {
            m_decoder->reader.Reset();
        }
#endif
        if (m_textureId != 0)
        {
            glDeleteTextures(1, &m_textureId);
            m_textureId = 0;
        }
    }

    bool VideoClip::IsReady() const
    {
        return m_initialized && m_textureId != 0 && m_width > 0 && m_height > 0;
    }

    void VideoClip::Update(double deltaSeconds)
    {
#ifdef PLATFORM_WINDOWS
        if (!m_initialized || !m_decoder || !m_decoder->reader)
        {
            return;
        }

        if (deltaSeconds < 0.0)
        {
            deltaSeconds = 0.0;
        }

        m_timeAccumulator += deltaSeconds;
        if (m_timeAccumulator < m_secondsPerFrame)
        {
            return;
        }

        const double leftover = std::fmod(m_timeAccumulator, m_secondsPerFrame);
        if (!DecodeNextSample())
        {
            if (m_loop && RestartStream())
            {
                DecodeNextSample();
            }
        }
        m_timeAccumulator = leftover;
#else
        (void)deltaSeconds;
#endif
    }

    void VideoClip::Restart()
    {
#ifdef PLATFORM_WINDOWS
        if (RestartStream())
        {
            DecodeNextSample();
            m_timeAccumulator = 0.0;
        }
#endif
    }

    void VideoClip::Initialize(const std::string& absolutePath)
    {
#ifdef PLATFORM_WINDOWS
        if (absolutePath.empty())
        {
            LOGE("VideoClip: Cannot load video with empty path");
            return;
        }

        try
        {
            EnsureMediaFoundation();
        }
        catch (const std::exception& e)
        {
            LOGE("VideoClip: Media Foundation init failed: %s", e.what());
            return;
        }

        std::wstring widePath = ToWide(absolutePath);
        if (widePath.empty())
        {
            LOGE("VideoClip: Failed to convert path '%s' to wide string", absolutePath.c_str());
            return;
        }

        Microsoft::WRL::ComPtr<IMFAttributes> attributes;
        HRESULT hr = MFCreateAttributes(&attributes, 1);
        if (FAILED(hr))
        {
            LOGE("VideoClip: MFCreateAttributes failed (0x%08X)", hr);
            return;
        }

        attributes->SetUINT32(MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, TRUE);
        attributes->SetUINT32(MF_SOURCE_READER_ENABLE_VIDEO_PROCESSING, TRUE);

        hr = MFCreateSourceReaderFromURL(widePath.c_str(), attributes.Get(), m_decoder->reader.GetAddressOf());
        if (FAILED(hr))
        {
            LOGE("VideoClip: MFCreateSourceReaderFromURL failed for '%s' (0x%08X)", absolutePath.c_str(), hr);
            return;
        }

        m_decoder->reader->SetStreamSelection(static_cast<DWORD>(MF_SOURCE_READER_ALL_STREAMS), FALSE);
        m_decoder->reader->SetStreamSelection(static_cast<DWORD>(MF_SOURCE_READER_FIRST_VIDEO_STREAM), TRUE);

        Microsoft::WRL::ComPtr<IMFMediaType> mediaType;
        hr = MFCreateMediaType(&mediaType);
        if (FAILED(hr))
        {
            LOGE("VideoClip: MFCreateMediaType failed (0x%08X)", hr);
            return;
        }

        mediaType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
        mediaType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32);

        hr = m_decoder->reader->SetCurrentMediaType(static_cast<DWORD>(MF_SOURCE_READER_FIRST_VIDEO_STREAM), nullptr, mediaType.Get());
        if (FAILED(hr))
        {
            LOGE("VideoClip: SetCurrentMediaType failed (0x%08X)", hr);
            return;
        }

        Microsoft::WRL::ComPtr<IMFMediaType> outputType;
        hr = m_decoder->reader->GetCurrentMediaType(static_cast<DWORD>(MF_SOURCE_READER_FIRST_VIDEO_STREAM), &outputType);
        if (FAILED(hr))
        {
            LOGE("VideoClip: GetCurrentMediaType failed (0x%08X)", hr);
            return;
        }

        UINT32 width = 0;
        UINT32 height = 0;
        if (FAILED(MFGetAttributeSize(outputType.Get(), MF_MT_FRAME_SIZE, &width, &height)))
        {
            LOGE("VideoClip: Failed to read frame size metadata");
            return;
        }

        UINT32 frameRateNumerator = 0;
        UINT32 frameRateDenominator = 0;
        if (SUCCEEDED(MFGetAttributeRatio(outputType.Get(), MF_MT_FRAME_RATE, &frameRateNumerator, &frameRateDenominator)) &&
            frameRateNumerator != 0 && frameRateDenominator != 0)
        {
            m_secondsPerFrame = static_cast<double>(frameRateDenominator) / static_cast<double>(frameRateNumerator);
        }
        else
        {
            m_secondsPerFrame = 1.0 / 30.0;
        }

        m_width = width;
        m_height = height;
        m_frameBuffer.resize(static_cast<size_t>(m_width) * static_cast<size_t>(m_height) * 4);

        glGenTextures(1, &m_textureId);
        glBindTexture(GL_TEXTURE_2D, m_textureId);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_width, m_height, 0, 0x80E1 /*GL_BGRA*/, GL_UNSIGNED_BYTE, nullptr);
        glBindTexture(GL_TEXTURE_2D, 0);

        m_initialized = DecodeNextSample();
#else
        LOGW("VideoClip: Runtime video playback is not supported on this platform");
        (void)absolutePath;
#endif
    }

    bool VideoClip::DecodeNextSample()
    {
#ifdef PLATFORM_WINDOWS
        if (!m_decoder || !m_decoder->reader)
        {
            return false;
        }

        while (true)
        {
            Microsoft::WRL::ComPtr<IMFSample> sample;
            DWORD streamIndex = 0;
            DWORD flags = 0;
            LONGLONG timestamp = 0;

            HRESULT hr = m_decoder->reader->ReadSample(
                static_cast<DWORD>(MF_SOURCE_READER_FIRST_VIDEO_STREAM),
                0,
                &streamIndex,
                &flags,
                &timestamp,
                &sample);

            if (FAILED(hr))
            {
                LOGE("VideoClip: ReadSample failed (0x%08X)", hr);
                return false;
            }

            if (flags & MF_SOURCE_READERF_ENDOFSTREAM)
            {
                m_reachedEnd = true;
                return false;
            }

            if (flags & MF_SOURCE_READERF_STREAMTICK)
            {
                continue;
            }

            if (!sample)
            {
                continue;
            }

            Microsoft::WRL::ComPtr<IMFMediaBuffer> mediaBuffer;
            hr = sample->ConvertToContiguousBuffer(&mediaBuffer);
            if (FAILED(hr))
            {
                LOGE("VideoClip: ConvertToContiguousBuffer failed (0x%08X)", hr);
                return false;
            }

            BYTE* data = nullptr;
            DWORD maxLength = 0;
            DWORD currentLength = 0;
            hr = mediaBuffer->Lock(&data, &maxLength, &currentLength);
            if (FAILED(hr))
            {
                LOGE("VideoClip: Buffer lock failed (0x%08X)", hr);
                return false;
            }

            const size_t expectedBytes = static_cast<size_t>(m_width) * static_cast<size_t>(m_height) * 4;
            if (m_frameBuffer.size() < expectedBytes)
            {
                m_frameBuffer.resize(expectedBytes);
            }

            std::memcpy(m_frameBuffer.data(), data, std::min(expectedBytes, static_cast<size_t>(currentLength)));
            mediaBuffer->Unlock();

            UploadFrame(m_frameBuffer);
            m_reachedEnd = false;
            return true;
        }
#else
        return false;
#endif
    }

    bool VideoClip::RestartStream()
    {
#ifdef PLATFORM_WINDOWS
        if (!m_decoder || !m_decoder->reader)
        {
            return false;
        }

        PROPVARIANT position;
        PropVariantInit(&position);
        position.vt = VT_I8;
        position.hVal.QuadPart = 0;

        HRESULT hr = m_decoder->reader->SetCurrentPosition(GUID_NULL, position);
        PropVariantClear(&position);

        if (FAILED(hr))
        {
            LOGE("VideoClip: Failed to reset stream position (0x%08X)", hr);
            return false;
        }

        m_reachedEnd = false;
        return true;
#else
        return false;
#endif
    }

    void VideoClip::UploadFrame(const std::vector<uint8_t>& frameData)
    {
#ifdef PLATFORM_WINDOWS
        if (m_textureId == 0 || m_width == 0 || m_height == 0)
        {
            return;
        }

        glBindTexture(GL_TEXTURE_2D, m_textureId);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_width, m_height, 0x80E1 /*GL_BGRA*/, GL_UNSIGNED_BYTE, frameData.data());
        glBindTexture(GL_TEXTURE_2D, 0);
#else
        (void)frameData;
#endif
    }

#ifdef PLATFORM_WINDOWS
    void VideoClip::EnsureMediaFoundation()
    {
        static std::once_flag s_once;
        static HRESULT s_result = S_OK;
        std::call_once(s_once, []()
            {
                s_result = MFStartup(MF_VERSION, MFSTARTUP_FULL);
                if (SUCCEEDED(s_result))
                {
                    std::atexit(ShutdownMediaFoundation);
                }
                else
                {
                    LOGE("VideoClip: MFStartup failed (0x%08X)", s_result);
                }
            });

        if (FAILED(s_result))
        {
            throw std::runtime_error("Media Foundation startup failed");
        }
    }

    std::wstring VideoClip::ToWide(const std::string& path)
    {
        if (path.empty())
        {
            return {};
        }

        int required = MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, nullptr, 0);
        if (required <= 0)
        {
            return {};
        }

        std::wstring result(static_cast<size_t>(required), L'\0');
        MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, result.data(), required);
        if (!result.empty() && result.back() == L'\0')
        {
            result.pop_back();
        }
        return result;
    }
#endif
}
