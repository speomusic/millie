#pragma once

#include <stdexcept>
#include "merc/vst/ivstvideoprocessor.h"

#if defined(_WIN32)
#include <d3d12.h>
#elif defined(__APPLE__) && defined(__OBJC__)
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>

@class MercVideo;
#endif

namespace merc::av
{
    struct Out;
}

namespace merc::vst
{
    struct Factory;
}

namespace merc::av::realtime
{
    struct VideoImplementation
    {
        VideoImplementation(Out& mainOut, vst::Factory& factory);
        ~VideoImplementation() = default;
        operator bool() const;
#if defined(_WIN32)

#elif defined(__APPLE__) && defined(__OBJC__)
        MercVideo* realtimeVideo;
        NSWindow* window;
        MTKView* view;
#endif
    };
}
