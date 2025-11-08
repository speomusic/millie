#pragma once

#include <string>
#include <filesystem>
#include <pluginterfaces/vst/ivstcomponent.h>
#include "merc/av/element.h"
#include "merc/av/work.h"

namespace merc::av
{
    template<typename T>
    struct ChangeScope
    {
        ChangeScope(struct Merc& merc);
        ChangeScope(const ChangeScope&) = delete;
        ChangeScope(ChangeScope&&) = delete;
        ChangeScope& operator=(const ChangeScope&) = delete;
        ChangeScope& operator=(ChangeScope&&) = delete;
        ~ChangeScope();
        struct Merc& merc;
        void addChain(std::string chainName);
        unsigned addElement(const std::string& chainName, const T& element);
        void insertElement(structure::Index index, const T& element);
        void removeElement(structure::Index index);
        void changeRouting(structure::Index index, typename ElementTraits<T>::Routing routing);
    private:
        friend ElementImplementation<T, structure::ChildMercRunner>;
        void onMercElementRoutingAndMediaChanged();
    };
}
