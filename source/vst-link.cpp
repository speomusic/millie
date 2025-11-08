#include "merc/av/link.h"
#include "merc/av/merc-av.h"
#include "merc/vst/vst-utils.h"
#include "merc/global/config.h"
#include "visitors.h"

namespace merc::av
{
    template<> Samples createResource<structure::Link>(Steinberg::Vst::SpeakerArrangement arrangement, const Merc& merc)
    {
        return Samples{ 2, global::getConfig().audio.batchSize };
    }

    template<> Steinberg::Vst::SpeakerArrangement getMainOutArrangement<structure::Link>()
    {
        return Steinberg::Vst::SpeakerArr::kStereo;
    }

    VstElementImplementation<structure::Link>::VstElementImplementation(ElementImplementation<structure::Link, structure::VstRunner>& ve)
        : vstElement{ ve }
    {}

    void VstElementImplementation<structure::Link>::setupForMerc()
    {
        if (vstElement.canReceiveEvents())
        {
            vstElement.inEvents = std::make_unique<Steinberg::Vst::EventList>(global::getConfig().audio.eventBufferSize);
            vstElement.eventGate = std::make_unique<Gate<TimedEvent>>(global::getConfig().audio.eventBufferSize);
        }
        Steinberg::Vst::ProcessSetup setup
        {
            Steinberg::Vst::kRealtime,
            Steinberg::Vst::kSample32,
            static_cast<int>(global::getConfig().audio.batchSize),
            static_cast<Steinberg::Vst::SampleRate>(global::getConfig().audio.sampleRate)
        };
        CHECK_TRESULT(vstElement.processor->setupProcessing(setup));
    }

    void VstElementImplementation<structure::Link>::setBusArrangement(std::vector<Steinberg::Vst::SpeakerArrangement>& inArrangements,
                                                                      std::vector<Steinberg::Vst::SpeakerArrangement>& outArrangements)
    {
        CHECK_TRESULT(vstElement.processor->setBusArrangements(inArrangements.data(), (int)inArrangements.size(), outArrangements.data(), (int)outArrangements.size()));
    }

    void VstElementImplementation<structure::Link>::postActivate()
    {
        CHECK_TRESULT(vstElement.processor->setProcessing(true));
    }

    void VstElementImplementation<structure::Link>::preDeactivate()
    {
        CHECK_TRESULT(vstElement.processor->setProcessing(false));
    }

    void VstElementImplementation<structure::Link>::setupProcessData()
    {
        int totalNumChannels{ 0 };
        for (const auto& in : vstElement.element.media.ins)
            totalNumChannels += std::visit(InChannelCount{ vstElement.element.merc }, in);
        for (const auto& out : vstElement.element.media.outs)
            totalNumChannels += std::visit(OutChannelCount{ vstElement.element.merc }, out);
        samples.resize(totalNumChannels);

        int channelIndex{ 0 };
        inAudioBusBuffers.resize(vstElement.element.media.ins.size());
        for (int i{ 0 }; i < vstElement.element.media.ins.size(); ++i)
        {
            inAudioBusBuffers[i].numChannels = std::visit(InChannelCount{ vstElement.element.merc }, vstElement.element.media.ins[i]);
            inAudioBusBuffers[i].channelBuffers32 = samples.data() + channelIndex;
            channelIndex += inAudioBusBuffers[i].numChannels;
        }
        outAudioBusBuffers.resize(vstElement.element.media.outs.size());
        for (int i{ 0 }; i < vstElement.element.media.outs.size(); ++i)
        {
            outAudioBusBuffers[i].numChannels = std::visit(OutChannelCount{ vstElement.element.merc }, vstElement.element.media.outs[i]);
            outAudioBusBuffers[i].channelBuffers32 = samples.data() + channelIndex;
            channelIndex += outAudioBusBuffers[i].numChannels;
        }

        processData.numSamples = global::getConfig().audio.batchSize;
        processData.numInputs = (int)vstElement.element.routing.ins.size();
        processData.numOutputs = (int)vstElement.element.routing.outs.size();
        processData.inputs = inAudioBusBuffers.data();
        processData.outputs = outAudioBusBuffers.data();
        processData.inputEvents = vstElement.inEvents.get();
    }

    void VstElementImplementation<structure::Link>::run(uint64_t iterationIndex)
    {
        for (int i{ 0 }; i < vstElement.element.media.ins.size(); ++i)
            std::visit(InSetChannelData{ vstElement.element.merc, inAudioBusBuffers[i], iterationIndex % 2 }, vstElement.element.media.ins[i]);
        for (int i{ 0 }; i < vstElement.element.media.outs.size(); ++i)
            std::visit(OutSetChannelData{ vstElement.element.merc, outAudioBusBuffers[i], (iterationIndex + 1) % 2 }, vstElement.element.media.outs[i]);
        vstElement.processor->process(processData);
    }
}
