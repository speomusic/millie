#pragma once

#include <memory>
#include <atomic>
#include <chrono>
#include <thread>
#include "merc/av/out.h"
#include "merc/vst/factory.h"

namespace merc::av::realtime
{
    struct VideoImplementation;

    struct VideoThread
    {
        void operator()();
        std::shared_ptr<VideoImplementation> implementation;
        std::atomic_flag* cancelled;
        const std::chrono::duration<double, std::milli> timePerFrame{};
    };

    struct Video
    {
        Video(Out& mainOut, vst::Factory& factory);
        ~Video();
        operator bool() const;
        std::shared_ptr<VideoImplementation> implementation;
    private:
        std::atomic_flag realtimeThreadCancelled;
        std::thread realtimeThread;
    };
}
