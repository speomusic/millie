#include "merc/av/resource.h"
#include <assert.h>

namespace merc::av
{
    Samples::Samples(size_t numChannels, size_t numFrames)
        : data(numChannels * numFrames, 0.0f)
    {
        channels.reserve(numChannels);
        for (int i{ 0 }; i < numChannels; ++i)
            channels.push_back({ data.data() + i * numFrames, numFrames });
    }

    Samples::Samples(const Samples& other)
        : Samples(other.channels.size(), other.channels.front().size())
    {
        std::copy(other.data.begin(), other.data.end(), data.begin());
    }

    Samples& Samples::operator=(const Samples& other)
    {
        assert(data.size() == other.data.size());
        assert(channels.size() == other.channels.size());
        std::copy(other.data.begin(), other.data.end(), data.begin());
        return *this;
    }
}
