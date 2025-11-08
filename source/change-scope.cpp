#include "merc/av/change-scope.h"
#include "merc/av/worker.h"
#include "merc/av/merc-av.h"

namespace merc::av
{
    template<typename T>
    ChangeScope<T>::ChangeScope(struct Merc& m)
        : merc{ m }
    {
        std::get<Work<Element<T>>>(merc.works).block();
    }

    template<typename T>
    ChangeScope<T>::~ChangeScope()
    {
        std::get<Work<Element<T>>>(merc.works).unblock();
    }

    template<typename T>
    void ChangeScope<T>::addChain(std::string chainName)
    {
        std::get<Work<Element<T>>>(merc.works).add(std::move(chainName), std::make_unique<Chain<Element<T>>>());
    }

    template<typename T>
    unsigned ChangeScope<T>::addElement(const std::string& chainName, const T& element)
    {
        structure::Index index{ chainName, std::get<Work<Element<T>>>(merc.works).size(chainName) };
        auto avElement{ std::make_unique<Element<T>>(element.runner, element.routing, index, merc) };
        std::get<Work<Element<T>>>(merc.works).add(chainName, std::move(avElement));
        if (std::holds_alternative<structure::ChildMercRunner>(element.runner))
            if (!merc.getChildMercs<Element<T>>().try_emplace(index, std::make_unique<Merc>(ParentMerc{ merc, index })).second)
                throw "Child merc shouldn't exist for this index already.";
        return index.element;
    }

    template<typename T>
    void ChangeScope<T>::insertElement(structure::Index index, const T& element)
    {
        auto& work{ std::get<Work<Element<T>>>(merc.works) };
        if (work.size(index.chain) == index.element)
        {
            addElement(index.chain, element);
            return;
        }
        auto avElement{ std::make_unique<Element<T>>(element.runner, element.routing, index, merc) };
        work.insert(index, std::move(avElement));
        // redo the routing of the following element to plug in ChainIns
        structure::Index nextIndex{ index.chain, index.element + 1 };
        changeRouting(nextIndex, work.at(nextIndex).getRouting());
        // fix up child merc indices
        auto& childMercs{ merc.getChildMercs<Element<T>>() };
        ChildMercs newChildMercs; newChildMercs.reserve(childMercs.size());
        for (auto it{ childMercs.begin() }; it != childMercs.end();)
        {
            auto extractIt{ it++ };
            auto nh{ childMercs.extract(extractIt) };
            if (it->first.chain == index.chain && it->first.element >= index.element) ++nh.key().element;
            newChildMercs.insert(std::move(nh));
        }
        childMercs = std::move(newChildMercs);
        if (std::holds_alternative<structure::ChildMercRunner>(element.runner))
            if (!childMercs.try_emplace(index, std::make_unique<Merc>(ParentMerc{ merc, index })).second)
                throw "Child merc shouldn't exist for this index already.";
    }

    template<typename T>
    void ChangeScope<T>::removeElement(structure::Index index)
    {
        std::get<Work<Element<T>>>(merc.works).remove(index);
    }

    template<typename T>
    void ChangeScope<T>::changeRouting(structure::Index index, typename ElementTraits<T>::Routing routing)
    {
        std::get<Work<Element<T>>>(merc.works).at(index).changeRouting(routing);
    }

    template<typename T>
    void ChangeScope<T>::onMercElementRoutingAndMediaChanged()
    {
        std::get<Work<Element<T>>>(merc.works).onMercElementRoutingAndMediaChanged();
    }

    template struct ChangeScope<structure::Link>;
    template struct ChangeScope<structure::Step>;
}
