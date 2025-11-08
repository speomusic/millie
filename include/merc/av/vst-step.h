#pragma once

#include "merc/av/vst-element.h"

namespace merc::vst
{
    struct Graphics;
}

namespace merc::av
{
    template<> Pixels createResource<structure::Step>(structure::VideoBusArrangement arrangement, const Merc& merc);
    template<> structure::VideoBusArrangement getMainOutArrangement<structure::Step>();

    template<>
    struct VstElementImplementation<structure::Step>
    {
        VstElementImplementation(ElementImplementation<structure::Step, structure::VstRunner>& e);

        void setupForMerc();
        void setBusArrangement(std::vector<structure::VideoBusArrangement>& inArrangements,
                               std::vector<structure::VideoBusArrangement>& outArrangements);
        void postActivate() {}
        void preDeactivate() {}
        void setupProcessData();
        void run(uint64_t iterationIndex);

        ElementImplementation<structure::Step, structure::VstRunner>& vstElement;
        vst::Graphics* graphics{};
        std::vector<vst::VideoBusData> inVideoBusDatas, outVideoBusDatas;
        vst::VideoData processData;
    };
}
