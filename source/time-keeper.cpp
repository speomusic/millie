#include "merc/av/time-keeper.h"
#include "merc/av/link.h"
#include "merc/av/step.h"
#include "merc/global/config.h"

namespace merc::av
{
    template<typename Element> double getSamplesPerIterationImpl();
    template<> double getSamplesPerIterationImpl<Link>()
    {
        return (double)global::getConfig().audio.batchSize;
    }
    template<> double getSamplesPerIterationImpl<Step>()
    {
        return (1.0 / global::getConfig().video.framesPerSecond) * global::getConfig().audio.sampleRate;
    }

    template<typename Element> double getMessageSafetyDelayInSamples();
    template<> double getMessageSafetyDelayInSamples<Link>()
    {
        return uint64_t(global::getConfig().audio.messageSafetyDelayInBatches * getSamplesPerIterationImpl<Link>());
    }
    template<> double getMessageSafetyDelayInSamples<Step>()
    {
        return uint64_t(global::getConfig().video.messageSafetyDelayInFrames * getSamplesPerIterationImpl<Step>());
    }

    template<typename Element>
    void TimeKeeper<Element>::tryBumpIterationIndex(uint64_t newIterationIndex)
    {
        auto loadedIterationIndex{ iterationIndex.load(std::memory_order_relaxed) };
        while (newIterationIndex > loadedIterationIndex)
            iterationIndex.compare_exchange_weak(loadedIterationIndex,
                                                 newIterationIndex,
                                                 std::memory_order_seq_cst);
    }

    template<typename Element>
    uint64_t TimeKeeper<Element>::getCurrentIterationIndex() const
    {
        return iterationIndex.load(std::memory_order_seq_cst);
    }

    template<typename Element>
    uint64_t TimeKeeper<Element>::getCurrentSampleIndex() const
    {
        return getCurrentIterationIndex() * getSamplesPerIteration();
    }

    template<typename Element>
    double TimeKeeper<Element>::getSamplesPerIteration()
    {
        return getSamplesPerIterationImpl<Element>();
    }

    template<typename Element>
    uint64_t TimeKeeper<Element>::getClosestSafeSampleIndex() const
    {
        return getCurrentSampleIndex() + getMessageSafetyDelayInSamples<Element>();
    }

    template struct TimeKeeper<Link>;
    template struct TimeKeeper<Step>;
}
