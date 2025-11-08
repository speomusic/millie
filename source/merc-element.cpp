#include "merc/av/merc-element.h"
#include "merc/av/merc-av.h"

namespace merc::av
{
    template<typename T>
    ElementImplementation<T, structure::ChildMercRunner>::ElementImplementation(Element<T>& e, structure::ChildMercRunner runner)
        : element{ e }
    {}

    template<typename T>
    void ElementImplementation<T, structure::ChildMercRunner>::pushEvent(TimedEvent event)
    {
        auto& childMerc{ getChildMerc() };
        auto it{ childMerc.eventRouting.find(event.event.busIndex) };
        if (it == childMerc.eventRouting.end()) return;
        for (const auto& index : it->second)
            childMerc. template getWork<Element<T>>().at(index).pushEvent(event);
    }

    template<typename T>
    void ElementImplementation<T, structure::ChildMercRunner>::run(TimeKeeper<Element<T>>& timeKeeper)
    {
        auto& work{ getChildMerc(). template getWork<Element<T>>() };
        work.enter();
        work.iteration(timeKeeper, true);
        work.leave();
    }

    template<typename T>
    void ElementImplementation<T, structure::ChildMercRunner>::onRoutingAndMediaChanged()
    {
        getChildMerc(). template changeWork<T>().onMercElementRoutingAndMediaChanged();
    }

    template<typename T>
    Merc& ElementImplementation<T, structure::ChildMercRunner>::getChildMerc() const
    {
        return *element.merc. template getChildMercs<Element<T>>().at(element.index);
    }

    template struct ElementImplementation<structure::Link, structure::ChildMercRunner>;
    template struct ElementImplementation<structure::Step, structure::ChildMercRunner>;
}
