#pragma once

#include <atomic>

namespace merc::av
{
    template<typename Element>
    struct TimeKeeper
    {
        void tryBumpIterationIndex(uint64_t newIterationIndex);
        uint64_t getCurrentIterationIndex() const;
        uint64_t getCurrentSampleIndex() const;
        static double getSamplesPerIteration();
        uint64_t getClosestSafeSampleIndex() const;
    private:
        std::atomic<uint64_t> iterationIndex{};
    };
}
