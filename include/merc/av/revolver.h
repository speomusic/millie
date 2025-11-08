#pragma once

#include <atomic>
#include <vector>

namespace merc::av
{
    template<typename Bullet>
    struct Chamber
    {
        Chamber(Bullet init)
            : bullet{ std::move(init) }
        {}
        Chamber(const Chamber& other)
            : bullet{ other.bullet }
        {}
        Bullet bullet;
        std::atomic_flag loaded;
    };

    template<typename Bullet>
    struct Revolver;

    template<typename Bullet>
    struct Shot
    {
        Shot(Revolver<Bullet>& r);
        ~Shot();
        Revolver<Bullet>& revolver;
        const bool blank;
        const Bullet* const bullet;
    };

    template<typename Bullet>
    struct Revolver
    {
        Revolver(size_t size, Bullet init)
        {
            Chamber<Bullet> initChamber{ std::move(init) };
            chambers.reserve(size);
            while (chambers.size() < size - 1) chambers.emplace_back(initChamber);
            chambers.emplace_back(std::move(initChamber));
        }
        Shot<Bullet> fire()
        {
            return { *this };
        }
        Bullet& lock()
        {
            chambers[toLoad].loaded.wait(true, std::memory_order_acquire);
            return chambers[toLoad].bullet;
        }
        void load()
        {
            const auto previousToLoad{ toLoad };
            toLoad = (previousToLoad + 1) % chambers.size();
            chambers[previousToLoad].loaded.test_and_set(std::memory_order_release);
        }
        void unload()
        {
            for (auto& chamber : chambers)
            {
                if (!chamber.loaded.test(std::memory_order_relaxed))
                    continue;
                chamber.loaded.clear(std::memory_order_relaxed);
                chamber.loaded.notify_one();
            }
        }
        const Bullet& inspect()
        {
            return chambers.front().bullet;
        }
    private:
        friend struct Shot<Bullet>;
        unsigned position{ 0 }, toLoad{ 0 };
        std::vector<Chamber<Bullet>> chambers;
    };

    template<typename Bullet>
    Shot<Bullet>::Shot(Revolver<Bullet>& r)
        : revolver{ r }
        , blank{ !revolver.chambers[revolver.position].loaded.test(std::memory_order_acquire) }
        , bullet{ blank ? nullptr : &revolver.chambers[revolver.position].bullet }
    {}

    template<typename Bullet>
    Shot<Bullet>::~Shot()
    {
        if (blank) return;
        const auto previousPosition{ revolver.position };
        revolver.position = (previousPosition + 1) % revolver.chambers.size();
        revolver.chambers[previousPosition].loaded.clear(std::memory_order_relaxed);
        revolver.chambers[previousPosition].loaded.notify_one();
    }
}
