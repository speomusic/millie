#import <AppKit/AppKit.h>
#include "merc/av/step.h"
#include "merc/av/merc-av.h"
#include "merc/vst/vst-utils.h"
#include "merc/vst/graphics.h"
#include "merc/global/config.h"
#include "visitors.h"

namespace merc::av
{
    template<> Pixels createResource<structure::Step>(structure::VideoBusArrangement arrangement, const Merc& merc)
    {
        return { toVstVideoBusArrangement(arrangement).textureBus, *merc.factory.graphics };
    }

    template<> structure::VideoBusArrangement getMainOutArrangement<structure::Step>()
    {
        return structure::TextureBusArrangement
        {
            unsigned(global::getConfig().video.width * [NSScreen mainScreen].backingScaleFactor),
            unsigned(global::getConfig().video.height * [NSScreen mainScreen].backingScaleFactor),
            vst::kBGRA8Unorm
        };
    }

    VstElementImplementation<structure::Step>::VstElementImplementation(ElementImplementation<structure::Step, structure::VstRunner>& ve)
        : vstElement{ ve }
    {}

    void VstElementImplementation<structure::Step>::setupForMerc()
    {
        if (vstElement.canReceiveEvents())
        {
            vstElement.inEvents = std::make_unique<Steinberg::Vst::EventList>(global::getConfig().video.eventBufferSize);
            vstElement.eventGate = std::make_unique<Gate<TimedEvent>>(global::getConfig().video.eventBufferSize);
        }
        graphics = vstElement.element.merc.factory.graphics.get();
    }

    void VstElementImplementation<structure::Step>::setBusArrangement(std::vector<structure::VideoBusArrangement>& inArrangements,
                                                                      std::vector<structure::VideoBusArrangement>& outArrangements)
    {
        std::vector<vst::VideoBusArrangement> vstInArrangements, vstOutArrangements;
        vstInArrangements.reserve(inArrangements.size());
        for (const auto& in : inArrangements) vstInArrangements.push_back(toVstVideoBusArrangement(in));
        vstOutArrangements.reserve(outArrangements.size());
        for (const auto& out : outArrangements) vstOutArrangements.push_back(toVstVideoBusArrangement(out));
        CHECK_TRESULT(vstElement.processor->setBusArrangements(*graphics,
                                                            vstInArrangements.data(), (int)vstInArrangements.size(),
                                                            vstOutArrangements.data(), (int)vstOutArrangements.size()));
    }

    void VstElementImplementation<structure::Step>::setupProcessData()
    {
        inVideoBusDatas.resize(vstElement.element.media.ins.size());
        outVideoBusDatas.resize(vstElement.element.media.outs.size());
        processData.numInputs = (int)inVideoBusDatas.size();
        processData.numOutputs = (int)outVideoBusDatas.size();
        processData.inputs = inVideoBusDatas.data();
        processData.outputs = outVideoBusDatas.data();
        processData.inputEvents = vstElement.inEvents.get();
    }

    void VstElementImplementation<structure::Step>::run(uint64_t iterationIndex)
    {
        for (int i{ 0 }; i < vstElement.element.media.ins.size(); ++i)
            std::visit(InSetVideoBusData{ vstElement.element.merc, inVideoBusDatas[i], iterationIndex % 2 }, vstElement.element.media.ins[i]);
        for (int i{ 0 }; i < vstElement.element.media.outs.size(); ++i)
            std::visit(OutSetVideoBusData{ vstElement.element.merc, outVideoBusDatas[i], (iterationIndex + 1) % 2 }, vstElement.element.media.outs[i]);
        vstElement.processor->process(*graphics, processData);
    }
}
