#pragma once

#include <vector>
#include <unordered_map>
#include <atomic>
#include <memory>
#include <cstdint>
#include "merc/structure/value.h"
#include "merc/global/config.h"
#include "merc/av/time-keeper.h"

namespace merc::av
{
    template<typename Element>
    struct Chain
    {
        Chain() = default;

        Chain(std::vector<std::unique_ptr<Element>> es)
            : elements{ std::move(es) }
        {}

        uint64_t tryRun(TimeKeeper<Element>& timeKeeper)
        {
            const auto loadedBusy{ busy.test_and_set(std::memory_order_seq_cst) };
            auto loadedIterationIndex{ iterationIndex.load(std::memory_order_seq_cst) };
            if (loadedBusy) return loadedIterationIndex;
            if (timeKeeper.getCurrentIterationIndex() >= loadedIterationIndex)
            {
                run(timeKeeper);
                iterationIndex.store(++loadedIterationIndex, std::memory_order_seq_cst);
            }
            busy.clear(std::memory_order_seq_cst);
            return loadedIterationIndex;
        }

        bool isUpchain() const
        {
            for (const auto& element : elements)
                if (element->hasMercInOrOutRouting())
                    return true;
            return false;
        }

        void onMercElementRoutingAndMediaChanged()
        {
            for (const auto& element : elements)
                if (element->hasMercInOrOutRouting())
                    element->onRoutingAndMediaChanged();
        }

        std::vector<std::unique_ptr<Element>> elements;
        std::atomic_flag busy;
        std::atomic<uint64_t> iterationIndex{ 0 };
    private:
        void run(TimeKeeper<Element>& timeKeeper)
        {
            for (auto& element : elements)
                element->run(timeKeeper);
        }
    };

    struct WorkSync
    {
        WorkSync()
        {
            ctrl.store(0, std::memory_order_relaxed);
        }
        void block()
        {
            auto lctrl{ ctrl.load(std::memory_order_relaxed) };
            while (!ctrl.compare_exchange_weak(lctrl, asBlocked(lctrl), std::memory_order_relaxed));
            lctrl = ctrl.load(std::memory_order_relaxed);
            while (areWorkersWorking(lctrl))
            {
                ctrl.wait(lctrl, std::memory_order_relaxed);
                lctrl = ctrl.load(std::memory_order_relaxed);
            }
        }
        void unblock()
        {
            auto lctrl{ ctrl.load(std::memory_order_relaxed) };
            while (!ctrl.compare_exchange_weak(lctrl, asUnblocked(lctrl), std::memory_order_release, std::memory_order_relaxed));
            ctrl.notify_all();
        }
        void onWorkerEntersIteration()
        {
            auto lctrl{ ctrl.load(std::memory_order_relaxed) };
            do { waitForUnblock(lctrl); }
            while (!ctrl.compare_exchange_strong(lctrl, withAdditionalWorkerWorking(lctrl), std::memory_order_acquire, std::memory_order_relaxed));
        }
        void onWorkerLeavesIteration()
        {
            bool shouldNotify{ false };
            auto lctrl{ ctrl.load(std::memory_order_relaxed) };
            while (!ctrl.compare_exchange_weak(lctrl, withOneLessWorkerWorking(lctrl, shouldNotify), std::memory_order_relaxed));
            if (shouldNotify) ctrl.notify_all();
        }
    private:
        void waitForUnblock(uint32_t& lctrl)
        {
            while (isBlocked(lctrl))
            {
                ctrl.wait(lctrl, std::memory_order_relaxed);
                lctrl = ctrl.load(std::memory_order_relaxed);
            }
        }
        uint32_t asBlocked(uint32_t lctrl)
        {
            return lctrl | 0x80'00'00'00;
        }
        uint32_t asUnblocked(uint32_t lctrl)
        {
            return lctrl & 0x7F'FF'FF'FF;
        }
        bool isBlocked(uint32_t lctrl)
        {
            return lctrl & 0x80'00'00'00;
        }
        uint32_t withAdditionalWorkerWorking(uint32_t lctrl)
        {
            return lctrl + 1;
        }
        uint32_t withOneLessWorkerWorking(uint32_t lctrl, bool& shouldNotify)
        {
            shouldNotify = (asUnblocked(--lctrl) == 0) && isBlocked(lctrl);
            return lctrl;
        }
        bool areWorkersWorking(uint32_t lctrl)
        {
            return asUnblocked(lctrl) > 0;
        }
        std::atomic<uint32_t> ctrl;
    };

    template<typename Element>
    struct Work
    {
        Work()
            : sync{ std::make_unique<WorkSync>() }
        {}

        unsigned size() const
        {
            return (unsigned)chains.size();
        }

        unsigned size(const std::string& chainName) const
        {
            return (unsigned)chains.at(chainName)->elements.size();
        }

        void add(std::string chainName, std::unique_ptr<Chain<Element>> chain)
        {
            chains.try_emplace(std::move(chainName), std::move(chain));
        }

        void add(const std::string& chainName, std::unique_ptr<Element> element)
        {
            chains.at(chainName)->elements.emplace_back(std::move(element));
        }

        void insert(structure::Index index, std::unique_ptr<Element> element)
        {
            auto& chain{ chains.at(index.chain) };
            chain->elements.emplace(chain->elements.begin() + index.element, std::move(element));
            for (unsigned elementIndex{ index.element + 1 }; elementIndex < chain->elements.size(); ++elementIndex)
                chain->elements[elementIndex]->bumpElementIndexAfterInsert();
        }

        void remove(structure::Index index)
        {
            auto& chain{ chains.at(index.chain) };
            chain->elements.erase(chain->elements.begin() + index.element);
        }

        Element& at(structure::Index index)
        {
            return *chains.at(index.chain)->elements.at(index.element);
        }

        const Element& at(structure::Index index) const
        {
            return *chains.at(index.chain)->elements.at(index.element);
        }

        void block()
        {
            sync->block();
        }

        void unblock()
        {
            sync->unblock();
        }

        void enter()
        {
            sync->onWorkerEntersIteration();
        }

        void leave()
        {
            sync->onWorkerLeavesIteration();
        }

        void onMercElementRoutingAndMediaChanged()
        {
            for (auto& [_, chain] : chains)
                chain->onMercElementRoutingAndMediaChanged();
        }

        std::optional<uint64_t> iteration(TimeKeeper<Element>& timeKeeper, bool upchain)
        {
            std::optional<uint64_t> lowestIterationIndex{};
            for (auto& [_, chain] : chains)
            {
                if (upchain != chain->isUpchain()) continue;
                const auto chainIterationIndex{ chain->tryRun(timeKeeper) };
                if (!lowestIterationIndex.has_value()) lowestIterationIndex = chainIterationIndex;
                else lowestIterationIndex = std::min(lowestIterationIndex.value(), chainIterationIndex);
            }
            return lowestIterationIndex;
        }
    private:
        template<typename U> struct ChangeScope;
        std::unordered_map<std::string, std::unique_ptr<Chain<Element>>> chains;
        std::unique_ptr<WorkSync> sync;
    };
}
