#pragma once

#include "merc/av/vst-element.h"

namespace merc::av
{
    template<> Samples createResource<structure::Link>(Steinberg::Vst::SpeakerArrangement arrangement, const struct Merc& merc);
    template<> Steinberg::Vst::SpeakerArrangement getMainOutArrangement<structure::Link>();

    template<>
    struct VstElementImplementation<structure::Link>
    {
        VstElementImplementation(ElementImplementation<structure::Link, structure::VstRunner>& ve);

        void setupForMerc();
        void setBusArrangement(std::vector<Steinberg::Vst::SpeakerArrangement>& inArrangements,
                               std::vector<Steinberg::Vst::SpeakerArrangement>& outArrangements);
        void postActivate();
        void preDeactivate();
        void setupProcessData();
        void run(uint64_t iterationIndex);

        ElementImplementation<structure::Link, structure::VstRunner>& vstElement;
        std::vector<float*> samples;
        std::vector<Steinberg::Vst::AudioBusBuffers> inAudioBusBuffers, outAudioBusBuffers;
        Steinberg::Vst::ProcessData processData;
    };
}
