#pragma once

#include <thread>
#include <tuple>
#include <vector>
#include <memory>
#include <algorithm>
#include "merc/av/merc-av.h"
#include "merc/av/thread.h"
#include "merc/av/time-keeper.h"

namespace merc::av
{
    template<typename Element>
    struct Thread
    {
        void operator()()
        {
            raiseCurrentThreadPriority();
            while (!cancelled->test(std::memory_order_relaxed))
            {
                const auto optionalLowestIterationIndex{ runIterations(rootMerc) };
                if (optionalLowestIterationIndex.has_value())
                    timeKeeper->tryBumpIterationIndex(optionalLowestIterationIndex.value());
            }
        }

        Merc* rootMerc;
        TimeKeeper<Element>* timeKeeper;
        std::atomic_flag* cancelled;
    private:
        std::optional<uint64_t> runIterations(Merc* merc) const
        {
            auto& work{ merc->getWork<Element>() };
            work.enter();
            auto optionalLowestIterationIndex{ work.iteration(*timeKeeper, false) };
            for (auto& [index, child] : merc->getChildMercs<Element>())
            {
                const auto optionalChildLowestIterationIndex{ runIterations(child.get()) };
                if (!optionalLowestIterationIndex.has_value())
                    optionalLowestIterationIndex = optionalChildLowestIterationIndex;
                else if (optionalChildLowestIterationIndex.has_value())
                    optionalLowestIterationIndex = std::min(optionalLowestIterationIndex.value(), optionalChildLowestIterationIndex.value());
            }
            work.leave();
            return optionalLowestIterationIndex;
        }
    };

    template<typename Element>
    struct Worker
    {
        Worker(Merc* rootMerc, TimeKeeper<Element>* timeKeeper)
            : thread
            {
                Thread<Element>
                {
                    rootMerc,
                    timeKeeper,
                    &cancelled
                }
            }
        {}
        ~Worker()
        {
            thread.join();
        }

    private:
        template<typename E>
        friend struct Workers;
        std::atomic_flag cancelled;
        std::thread thread;
    };

    template<typename Element>
    struct Workers
    {
        void start(int num, Merc* rootMerc)
        {
            for (int i{ 0 }; i < num; ++i)
                workers.emplace_back(new Worker<Element>{ rootMerc, &timeKeeper });
        }
        void cancel()
        {
            for (auto& worker : workers)
                worker->cancelled.test_and_set(std::memory_order_relaxed);
        }
        void clear()
        {
            workers.clear();
        }
    private:
        std::vector<std::unique_ptr<Worker<Element>>> workers;
        TimeKeeper<Element> timeKeeper;
    };
}
