#pragma once

#include <vector>
#include <span>
#include <memory>
#include <tuple>
#include <public.sdk/source/vst/hosting/eventlist.h>
#include "merc/vst/ivstvideoprocessor.h"

namespace merc::vst
{
    struct Stop;
    struct Texture;
    struct Graphics;
}

namespace merc::av
{
    struct Samples
    {
        Samples(size_t numChannels, size_t numFrames);
        ~Samples() = default;
        Samples(const Samples& other);
        Samples(Samples&& other) = default;
        Samples& operator=(const Samples& other);
        Samples& operator=(Samples&& other) = default;

        std::vector<float> data;
        std::vector<std::span<float>> channels;
    };

    using Events = Steinberg::Vst::EventList;

    struct Pixels
    {
        Pixels(const vst::TextureBusArrangement& arrangement, const struct vst::Graphics& graphics);
        Pixels(const Pixels& other);
        Pixels(Pixels&& other);
        Pixels& operator=(const Pixels& other) = delete;
        Pixels& operator=(Pixels&& other);
        ~Pixels();

        std::unique_ptr<vst::Texture> texture{};
    };
}
