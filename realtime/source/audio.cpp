#include <roapi.h>
#include <mmdeviceapi.h>
#include <wrl.h>
using Microsoft::WRL::ComPtr;
#include <audioclient.h>
#include <memory>
#include <thread>
#include <functional>
#include "merc/audio.h"
#include "merc/merc.h"
#include "merc/revolver.h"
#include "merc/resource.h"
#include "merc/windows-utils.h"

namespace merc
{
    struct RealtimeAudioImplementation
    {
        RealtimeAudioImplementation(std::shared_ptr<Revolver<Samples>> r, const Config::Audio& config);
        ~RealtimeAudioImplementation();
        UINT32 getNumBatchesToWrite() const;
        void writeBatch(float* interleavedDst, const Samples& batch) const;
        void writeZeroBatch(float* interleavedDst) const;
        void fillBufferWithBatches();

        const unsigned batchToBufferSizeFactor{ 8 };
        std::shared_ptr<Revolver<Samples>> revolver;
        const unsigned sampleRate, batchSize;

        ComPtr<IAudioClient> audioClient;
        ComPtr<IAudioRenderClient> renderClient;
        UINT32 numBufferFrames{};
        std::jthread audioRenderThread;

        friend struct AudioRenderer;
    };

    struct AudioRenderer
    {
        void operator()(std::stop_token stoken, RealtimeAudioImplementation* self);
    };

    RealtimeAudioImplementation::RealtimeAudioImplementation(std::shared_ptr<Revolver<Samples>> r, const Config::Audio& config)
        : revolver{ std::move(r) }
        , sampleRate{ config.sampleRate }
        , batchSize{ config.batchSize }
    {
        THROW_IF_FAILED(Windows::Foundation::Initialize(RO_INIT_SINGLETHREADED));
        ComPtr<IMMDeviceEnumerator> deviceEnumerator;
        THROW_IF_FAILED(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, IID_PPV_ARGS(&deviceEnumerator)));
        ComPtr<IMMDevice> audioDevice;
        THROW_IF_FAILED(deviceEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &audioDevice));
        THROW_IF_FAILED(audioDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, &audioClient));
        const DWORD streamFlags{ AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM | AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY };
        const WAVEFORMATEX waveFormat
        {
            .wFormatTag = WAVE_FORMAT_IEEE_FLOAT,
            .nChannels = 2,
            .nSamplesPerSec = (DWORD)sampleRate,
            .nAvgBytesPerSec = (DWORD)sampleRate * 8,
            .nBlockAlign = 8,
            .wBitsPerSample = 32
        };
        const auto requestedBufferTime
        {
            static_cast<REFERENCE_TIME>((batchSize * batchToBufferSizeFactor / (double)sampleRate) * 10'000'000)
        };
        THROW_IF_FAILED(audioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, streamFlags, requestedBufferTime, 0, &waveFormat, nullptr));
        THROW_IF_FAILED(audioClient->GetBufferSize(&numBufferFrames));
        THROW_IF_FAILED(audioClient->GetService(IID_PPV_ARGS(&renderClient)));
        audioRenderThread = std::jthread{ AudioRenderer{}, this};
    }

    RealtimeAudioImplementation::~RealtimeAudioImplementation()
    {
        audioRenderThread.request_stop();
    }

    UINT32 RealtimeAudioImplementation::getNumBatchesToWrite() const
    {
        UINT32 currentPadding;
        THROW_IF_FAILED(audioClient->GetCurrentPadding(&currentPadding));
        return (numBufferFrames - currentPadding) / batchSize;
    }

    void RealtimeAudioImplementation::writeBatch(float* interleavedDst, const Samples& batch) const
    {
        for (int i{ 0 }; i < batchSize; ++i)
        {
            *(interleavedDst + 0 + i * 2) = batch.channels[0][i];
            *(interleavedDst + 1 + i * 2) = batch.channels[1][i];
        }
    }

    void RealtimeAudioImplementation::writeZeroBatch(float* interleavedDst) const
    {
        for (int i{ 0 }; i < batchSize; ++i)
        {
            *(interleavedDst + 0 + i * 2) = 0.0f;
            *(interleavedDst + 1 + i * 2) = 0.0f;
        }
    }

    void RealtimeAudioImplementation::fillBufferWithBatches()
    {
        const auto numBatchesToWrite{ getNumBatchesToWrite() };
        if (numBatchesToWrite == 0) return;
        BYTE* data;
        {
            const auto hr{ renderClient->GetBuffer(numBatchesToWrite * batchSize, &data) };
            if (hr == AUDCLNT_E_BUFFER_TOO_LARGE) return fillBufferWithBatches();
            if (hr == AUDCLNT_E_DEVICE_INVALIDATED) return;
            if (FAILED(hr)) throw HrException{ hr, "Failed to get WASAPI Audio Render Client buffer." };
        }
        for (int i{ 0 }; i < numBatchesToWrite; ++i)
        {
            auto dest{ (float*)data + i * batchSize * 2 };
            auto shot{ revolver->fire() };
            if (!shot.blank)
                writeBatch(dest, *shot.bullet);
            else
                writeZeroBatch(dest);
        }
        {
            const auto hr{ renderClient->ReleaseBuffer(numBatchesToWrite * batchSize, 0) };
            if (hr == AUDCLNT_E_DEVICE_INVALIDATED) return;
            if (FAILED(hr)) throw HrException{ hr, "Failed to release WASAPI Audio Render Client buffer." };
        }
    }

    void AudioRenderer::operator()(std::stop_token stoken, RealtimeAudioImplementation* self)
    {
        if (!SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL))
            THROW_IF_FAILED(HRESULT_FROM_WIN32(GetLastError()));
        self->fillBufferWithBatches();
        THROW_IF_FAILED(self->audioClient->Start());
        while (!stoken.stop_requested()) self->fillBufferWithBatches();
        self->audioClient->Stop();
    }

    RealtimeAudio::RealtimeAudio(const Merc& merc)
        : implementation{ std::make_unique<RealtimeAudioImplementation>(std::get<SampleRevolver>(merc.outs), merc.config.audio) }
    {}
    RealtimeAudio::~RealtimeAudio() = default;
}
