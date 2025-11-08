#pragma once

#include <variant>
#include <vector>
#include <memory>
#include <array>
#include <pluginterfaces/vst/ivstaudioprocessor.h>
#include "merc/structure/merc-structure.h"
#include "merc/av/revolver.h"
#include "merc/av/resource.h"
#include "merc/av/time-keeper.h"

namespace merc::av
{
    template<typename T>
    struct ElementTraits;

    template<>
    struct ElementTraits<structure::Link>
    {
        using Routing = structure::LinkRouting;
        using Resource = Samples;
        using Arrangement = Steinberg::Vst::SpeakerArrangement;
        using In = structure::LinkIn;
        using Out = structure::LinkOut;
        using NextOut = structure::LinkNextOut;
        using ChainOut = structure::LinkChainOut;
        static constexpr int defaultMediaType{ Steinberg::Vst::kAudio };
    };

    template<>
    struct ElementTraits<structure::Step>
    {
        using Routing = structure::StepRouting;
        using Resource = Pixels;
        using Arrangement = structure::VideoBusArrangement;
        using In = structure::StepIn;
        using Out = structure::StepOut;
        using NextOut = structure::StepNextOut;
        using ChainOut = structure::StepChainOut;
        static constexpr int defaultMediaType{ vst::kVideo };
    };

    template<typename T, typename Routing> struct ElementRoutingTraits;

    template<typename T> struct ElementRoutingTraits<T, structure::NextIn>
    { using Medium = const typename ElementTraits<T>::Resource*; };
    template<typename T> struct ElementRoutingTraits<T, typename ElementTraits<T>::NextOut>
    { using Medium = typename ElementTraits<T>::Resource; };

    template<typename T> struct ElementRoutingTraits<T, structure::ChainIn>
    { using Medium = const std::array<typename ElementTraits<T>::Resource, 2>*; };
    template<typename T> struct ElementRoutingTraits<T, typename ElementTraits<T>::ChainOut>
    { using Medium = std::array<typename ElementTraits<T>::Resource, 2>; };

    template<typename T> struct ElementRoutingTraits<T, structure::MercIn>
    { using Medium = structure::UInt; }; // InMedium index of child merc element in parent merc
    template<typename T> struct ElementRoutingTraits<T, structure::MercOut>
    { using Medium = structure::UInt; }; // OutMedium index of child merc element in parent merc

    template<typename T> struct ElementRoutingTraits<T, structure::MainOut>
    { using Medium = std::shared_ptr<Revolver<typename ElementTraits<T>::Resource>>; };

    template<typename T, typename V> struct MediumImpl{};
    template<typename T, typename... Ts> struct MediumImpl<T, std::variant<Ts...>>
    { using type = std::variant<typename ElementRoutingTraits<T, Ts>::Medium...>; };

    template<typename T> using InMedium = typename MediumImpl<T, typename ElementTraits<T>::In>::type;
    template<typename T> using OutMedium = typename MediumImpl<T, typename ElementTraits<T>::Out>::type;

    template<typename T>
    struct Media
    {
        std::vector<InMedium<T>> ins;
        std::vector<OutMedium<T>> outs;
    };

    struct TimedEvent
    {
        size_t sample;
        Steinberg::Vst::Event event;
    };

    template<typename T> typename ElementTraits<T>::Resource createResource(typename ElementTraits<T>::Arrangement arrangement, const struct Merc& merc);
    template<typename T> typename ElementTraits<T>::Arrangement getMainOutArrangement();

    template<typename T, typename R>
    struct ElementImplementation;

    template<typename T>
    struct Element
    {
        Element(structure::Runner runner, typename ElementTraits<T>::Routing routing, structure::Index index, struct Merc& merc);
        Element(const Element&) = delete;
        Element(Element&&) = delete;
        Element& operator=(Element&&) = delete;
        Element& operator=(const Element&) = delete;
        void changeRouting(typename ElementTraits<T>::Routing newRouting);
        void pushEvent(TimedEvent event);
        void run(TimeKeeper<Element<T>>& timeKeeper);
        void onRoutingAndMediaChanged();
        const typename ElementTraits<T>::Routing& getRouting() const { return routing; }
        const Media<T>& getMedia() const { return media; }
        bool hasMercInOrOutRouting() const;
        void bumpElementIndexAfterInsert() { ++index.element; }

    private:
        std::vector<typename ElementTraits<T>::Arrangement> getInArrangements() const;
        std::vector<typename ElementTraits<T>::Arrangement> getOutArrangements() const;

        template <typename U, typename V> friend struct ElementImplementation;
        template <typename U> friend struct VstElementImplementation;
        template <typename U> friend struct InMediumCreator;
        template <typename U> friend struct ArrangementGetter;

        typename ElementTraits<T>::Routing routing;
        structure::Index index;
        Merc& merc;

        Media<T> media;

        using VstImpl = std::unique_ptr<ElementImplementation<T, structure::VstRunner>>;
        using MercImpl = std::unique_ptr<ElementImplementation<T, structure::ChildMercRunner>>;
        std::variant<VstImpl, MercImpl> impl;
    };
}
