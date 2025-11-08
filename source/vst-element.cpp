#include "merc/av/vst-element.h"
#include "merc/av/merc-av.h"
#include "merc/vst/vst-utils.h"
#include "visitors.h"

namespace merc::av
{
    template<typename T>
    ElementImplementation<T, structure::VstRunner>::ElementImplementation(Element<T>& e, structure::VstRunner runner)
        : element{ e }
        , cid{ runner.cid }
        , component{ element.merc.makeComponent(cid) }
        , processor{ Steinberg::FUnknownPtr<typename VstElementTraits<T>::Processor>{ component } }
        , impl{ *this }
    {
        impl.setupForMerc();
        impl.setupProcessData();
        activate();
    }

    template<typename T>
    void ElementImplementation<T, structure::VstRunner>::activate()
    {
        for (int i{ 0 }; i < component->getBusCount(Steinberg::Vst::kEvent, Steinberg::Vst::kInput); ++i)
            CHECK_TRESULT(component->activateBus(Steinberg::Vst::kEvent, Steinberg::Vst::kInput, i, true));

        for (int i{ 0 }; i < element.routing.ins.size(); ++i)
            CHECK_TRESULT(component->activateBus(ElementTraits<T>::defaultMediaType, Steinberg::Vst::kInput, i, true));
        for (int i{ 0 }; i < element.routing.outs.size(); ++i)
            CHECK_TRESULT(component->activateBus(ElementTraits<T>::defaultMediaType, Steinberg::Vst::kOutput, i, true));

        auto inArrangements{ element.getInArrangements() };
        auto outArrangements{ element.getOutArrangements() };
        impl.setBusArrangement(inArrangements, outArrangements);

        CHECK_TRESULT(component->setActive(true));
        impl.postActivate();
    }

    template<typename T>
    void ElementImplementation<T, structure::VstRunner>::deactivate()
    {
        impl.preDeactivate();
        CHECK_TRESULT(component->setActive(false));

        for (int i{ 0 }; i < element.routing.ins.size(); ++i)
            CHECK_TRESULT(component->activateBus(ElementTraits<T>::defaultMediaType, Steinberg::Vst::kInput, i, false));
        for (int i{ 0 }; i < element.routing.outs.size(); ++i)
            CHECK_TRESULT(component->activateBus(ElementTraits<T>::defaultMediaType, Steinberg::Vst::kOutput, i, false));
        
        for (int i{ 0 }; i < component->getBusCount(Steinberg::Vst::kEvent, Steinberg::Vst::kInput); ++i)
            CHECK_TRESULT(component->activateBus(Steinberg::Vst::kEvent, Steinberg::Vst::kInput, i, false));
    }

    template<typename T>
    ElementImplementation<T, structure::VstRunner>::~ElementImplementation()
    {
        if (!component) return;
        deactivate();
        component->terminate();
    }

    template<typename T>
    void ElementImplementation<T, structure::VstRunner>::pushEvent(TimedEvent event)
    {
        eventGate->push(event);
    }

    template<typename T>
    void ElementImplementation<T, structure::VstRunner>::pumpInputEvents(size_t startSampleIndex, size_t samplesPerRun)
    {
        while (auto* timedEvent{ eventGate->peak() })
        {
            if (timedEvent->sample >= startSampleIndex + samplesPerRun)
                break;
            if (timedEvent->sample > startSampleIndex)
                timedEvent->event.sampleOffset = Steinberg::int32(timedEvent->sample - startSampleIndex);
            inEvents->addEvent(timedEvent->event);
            eventGate->popPeaked();
        }
    }

    template<typename T>
    void ElementImplementation<T, structure::VstRunner>::run(const TimeKeeper<Element<T>>& timeKeeper)
    {
        const auto iterationIndex{ timeKeeper.getCurrentIterationIndex() };
        const auto samplesPerIteration{ TimeKeeper<Element<T>>::getSamplesPerIteration() };
        if (canReceiveEvents()) pumpInputEvents(size_t(iterationIndex * samplesPerIteration), size_t(samplesPerIteration));
        impl.run(iterationIndex);
        if (canReceiveEvents()) inEvents->clear();
        for (const auto& out : element.media.outs)
            std::visit(FinalizeRun<T>{ element.merc }, out);
    }

    template<typename T>
    void ElementImplementation<T, structure::VstRunner>::onRoutingAndMediaChanged()
    {
        deactivate();
        impl.setupProcessData();
        activate();
    }

    template<typename T>
    bool ElementImplementation<T, structure::VstRunner>::canReceiveEvents() const
    {
        return component->getBusCount(Steinberg::Vst::kEvent, Steinberg::Vst::kInput) > 0;
    }

    template struct ElementImplementation<structure::Link, structure::VstRunner>;
    template struct ElementImplementation<structure::Step, structure::VstRunner>;
}
