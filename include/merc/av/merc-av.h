#pragma once

#include <unordered_map>
#include <optional>
#include <public.sdk/source/vst/hosting/hostclasses.h>
#include <base/source/fobject.h>
#include "merc/vst/factory.h"
#include "merc/av/out.h"
#include "merc/av/space.h"
#include "merc/av/link.h"
#include "merc/av/step.h"
#include "merc/av/revolver.h"
#include "merc/av/resource.h"
#include "merc/av/work.h"
#include "merc/av/change-scope.h"

namespace merc::av
{
    using LinkChains = Work<Link>;
    using StepChains = Work<Step>;
    using ChildMercs = std::unordered_map<structure::Index, std::unique_ptr<Merc>>;

    struct ParentMerc
    {
        const struct Merc& merc;
        structure::Index index;
    };

    struct Merc : Steinberg::Vst::HostApplication
    {
        Merc(Out& mainOut, Space& space, vst::Factory& factory);
        Merc(ParentMerc parent);
        ~Merc();
        Merc(const Merc&) = delete;
        Merc& operator=(const Merc&) = delete;

        template<typename Element>
        Work<Element>& getWork() { return std::get<Work<Element>>(works); }
        LinkChains& getLinkChains() { return std::get<LinkChains>(works); }
        StepChains& getStepChains() { return std::get<StepChains>(works); }

        template<typename Element>
        const Work<Element>& getWork() const { return std::get<Work<Element>>(works); }
        const LinkChains& getLinkChains() const { return std::get<LinkChains>(works); }
        const StepChains& getStepChains() const { return std::get<StepChains>(works); }

        template<typename T>
        ChangeScope<T> changeWork() { return { *this }; }
        ChangeScope<structure::Link> changeLinkWork() { return { *this }; }
        ChangeScope<structure::Step> changeStepWork() { return { *this }; }

        template<typename Element> ChildMercs& getChildMercs();
        template<> ChildMercs& getChildMercs<Element<structure::Link>>() { return audioMercs; }
        template<> ChildMercs& getChildMercs<Element<structure::Step>>() { return videoMercs; }

        Steinberg::IPtr<Steinberg::Vst::IComponent> makeComponent(const std::string& cid);

        std::optional<ParentMerc> parent;
        Out& mainOut;
        Space& space;
        vst::Factory& factory;
        structure::EventRouting eventRouting;

        DEFINE_INTERFACES
            DEF_INTERFACE (IHostApplication)
        END_DEFINE_INTERFACES (Steinberg::Vst::HostApplication)
        REFCOUNT_METHODS (Steinberg::Vst::HostApplication)
    private:
        ChildMercs audioMercs, videoMercs;
        std::tuple<LinkChains, StepChains> works;

        bool tryLoad();
        Steinberg::tresult PLUGIN_API getName(Steinberg::Vst::String128 name) override;

        template<typename T> friend struct Element;
        template<typename T> friend struct InMediumCreator;
        template<typename T> friend struct OutMediumCreator;
        template<typename T> friend struct ArrangementGetter;
        template<typename T> friend struct ChangeScope;
    };
}
