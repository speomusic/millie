#pragma once

#include <atomic>
#include <vector>
#include <pluginterfaces/base/ftypes.h>

namespace merc
{
    template<typename T>
    struct Gate
    {
        Gate(unsigned bufferSize) : buffer(bufferSize) {}

        void push(T value)
        {
            unsigned loadedTo{ to.load(std::memory_order_relaxed) };
            buffer[loadedTo] = std::move(value);
            if (++loadedTo == buffer.size()) loadedTo = 0;
            to.store(loadedTo, std::memory_order_release);
        }

        T* peak()
        {
            unsigned loadedTo{ to.load(std::memory_order_acquire) };
            if (from == loadedTo) return nullptr;
            return &buffer[from];
        }

        void popPeaked()
        {
            if (++from == buffer.size()) from = 0;
        }
    private:
        std::vector<T> buffer;
        unsigned from{};
        std::atomic<unsigned> to{};
    };
}
