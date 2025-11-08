#pragma once

#include <public.sdk/source/vst/hosting/eventlist.h>
#include "merc/av/element.h"
#include "merc/av/gate.h"
#include "merc/vst/ivstvideoprocessor.h"

namespace merc::av
{
    template<typename T>
    struct VstElementTraits;

    template<>
    struct VstElementTraits<structure::Link>
    {
        using Processor = Steinberg::Vst::IAudioProcessor;
    };

    template<>
    struct VstElementTraits<structure::Step>
    {
        using Processor = vst::IVideoProcessor;
    };

    template<typename T>
    struct VstElementImplementation;

    template<typename T>
    struct ElementImplementation<T, structure::VstRunner>
    {
        ElementImplementation(Element<T>& e, structure::VstRunner runner);
        ElementImplementation(const ElementImplementation&) = delete;
        ElementImplementation(ElementImplementation&&) = delete;
        ElementImplementation& operator=(const ElementImplementation&) = delete;
        ElementImplementation& operator=(ElementImplementation&&) = delete;
        ~ElementImplementation();
        void onRoutingAndMediaChanged();
        void pushEvent(TimedEvent event);
        void run(const TimeKeeper<Element<T>>& timeKeeper);

        std::unique_ptr<Gate<TimedEvent>> eventGate;
    private:
        bool canReceiveEvents() const;
        void pumpInputEvents(size_t startSampleIndex, size_t samplesPerRun);
        void activate();
        void deactivate();

        template <typename U> friend struct VstElementImplementation;

        Element<T>& element;

        std::string cid;

        Steinberg::IPtr<Steinberg::Vst::IComponent> component;
        Steinberg::IPtr<typename VstElementTraits<T>::Processor> processor;

        std::unique_ptr<Steinberg::Vst::EventList> inEvents, outEvents;

        VstElementImplementation<T> impl;
    };
}
