#pragma once

#include <memory>
#include "merc/av/element.h"

namespace merc::av
{
    template<typename T>
    struct ElementImplementation<T, structure::ChildMercRunner>
    {
        ElementImplementation(Element<T>& e, structure::ChildMercRunner runner);
        void onRoutingAndMediaChanged();
        void pushEvent(TimedEvent event);
        void run(TimeKeeper<Element<T>>& timeKeeper);
    private:
        Element<T>& element;

        struct Merc& getChildMerc() const;
    };
}
