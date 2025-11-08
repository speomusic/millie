#include "merc/av/merc-av.h"
#include "merc/vst/vst-utils.h"
#include "merc/global/config.h"
#include <limits>

namespace merc::av
{
    Merc::Merc(Out& o, Space& s, vst::Factory& f)
        : mainOut{ o }
        , space{ s }
        , factory{ f }
    {}

    Merc::Merc(ParentMerc p)
        : parent{ std::move(p) }
        , mainOut{ parent.value().merc.mainOut }
        , space{ parent.value().merc.space }
        , factory{ parent.value().merc.factory }
    {}

    Merc::~Merc() = default;

    Steinberg::IPtr<Steinberg::Vst::IComponent> Merc::makeComponent(const std::string& cid)
    {
        auto component{ space.getComponent(cid) };
        component->initialize((IHostApplication*)this);
        return component;
    }

    Steinberg::tresult Merc::getName(Steinberg::Vst::String128 name)
    {
        static const auto hostName{ u"Merc" };
        std::char_traits<char16_t>::copy(name, hostName, std::char_traits<char16_t>::length(hostName) + 1);
        return Steinberg::kResultOk;
    }
}

