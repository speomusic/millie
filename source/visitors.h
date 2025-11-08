#pragma once

#include <pluginterfaces/vst/ivstaudioprocessor.h>
#include "merc/vst/ivstvideoprocessor.h"
#include "merc/av/merc-av.h"
#include "merc/av/revolver.h"
#include "merc/av/resource.h"
#include "merc/av/element.h"

namespace merc::av
{
    struct OutChannelCount
    {
        const Merc& merc;
        unsigned operator()(const Samples& samples) const
        {
            return (unsigned)samples.channels.size();
        }
        unsigned operator()(const std::array<Samples, 2>& doubleBuffer) const
        {
            return (unsigned)doubleBuffer.front().channels.size();
        }
        unsigned operator()(structure::UInt mercElemenOutIndex) const
        {
            const auto& parent{ merc.parent.value() };
            const auto& out{ parent.merc.getLinkChains().at(parent.index).getMedia().outs.at(mercElemenOutIndex) };
            return std::visit(OutChannelCount{ parent.merc }, out);
        }
        unsigned operator()(const std::shared_ptr<Revolver<Samples>>& revolver) const
        {
            return (unsigned)revolver->inspect().channels.size();
        }
    };

    struct InChannelCount
    {
        const Merc& merc;
        unsigned operator()(const Samples* samples) const
        {
            return OutChannelCount{ merc }(*samples);
        }
        unsigned operator()(const std::array<Samples, 2>* doubleBuffer) const
        {
            return OutChannelCount{ merc }(*doubleBuffer);
        }
        unsigned operator()(structure::UInt mercElementInIndex) const
        {
            const auto& parent{ merc.parent.value() };
            const auto& in{ parent.merc.getLinkChains().at(parent.index).getMedia().ins.at(mercElementInIndex) };
            return std::visit(InChannelCount{ parent.merc }, in);
        }
    };

    struct OutSetChannelData
    {
        const Merc& merc;
        Steinberg::Vst::AudioBusBuffers& audioBusBuffers;
        uint64_t doubleBufferIndex{ 0 };
        void operator()(const Samples& samples) const
        {
            for (int i{ 0 }; i < audioBusBuffers.numChannels; ++i)
                audioBusBuffers.channelBuffers32[i] = samples.channels[i].data();
        }
        void operator()(const std::array<Samples, 2>& doubleBuffer) const
        {
            (*this)(doubleBuffer[doubleBufferIndex]);
        }
        void operator()(structure::UInt mercElementOutIndex) const
        {
            const auto& parent{ merc.parent.value() };
            const auto& out{ parent.merc.getLinkChains().at(parent.index).getMedia().outs.at(mercElementOutIndex) };
            std::visit(OutSetChannelData{ parent.merc, audioBusBuffers, doubleBufferIndex }, out);
        }
        void operator()(const std::shared_ptr<Revolver<Samples>>& revolver) const
        {
            auto& bullet{ revolver->lock() };
            for (int i{ 0 }; i < audioBusBuffers.numChannels; ++i)
                audioBusBuffers.channelBuffers32[i] = bullet.channels[i].data();
        }
    };

    struct InSetChannelData
    {
        const Merc& merc;
        Steinberg::Vst::AudioBusBuffers& audioBusBuffers;
        uint64_t doubleBufferIndex{ 0 };
        void operator()(const Samples* samples) const
        {
            OutSetChannelData{ merc, audioBusBuffers, doubleBufferIndex }(*samples);
        }
        void operator()(const std::array<Samples, 2>* doubleBuffer) const
        {
            OutSetChannelData{ merc, audioBusBuffers, doubleBufferIndex }(*doubleBuffer);
        }
        void operator()(structure::UInt mercElementInIndex) const
        {
            const auto& parent{ merc.parent.value() };
            const auto& in{ parent.merc.getLinkChains().at(parent.index).getMedia().ins.at(mercElementInIndex) };
            std::visit(InSetChannelData{ parent.merc, audioBusBuffers, doubleBufferIndex }, in);
        }
    };

    struct OutSetVideoBusData
    {
        const Merc& merc;
        vst::VideoBusData& videoBusData;
        uint64_t doubleBufferIndex{ 0 };
        void operator()(const Pixels& pixels) const
        {
            videoBusData.texture = pixels.texture.get();
        }
        void operator()(const std::array<Pixels, 2>& doubleBuffer) const
        {
            (*this)(doubleBuffer[doubleBufferIndex]);
        }
        void operator()(structure::UInt mercElementOutIndex) const
        {
            const auto& parent{ merc.parent.value() };
            const auto& out{ parent.merc.getStepChains().at(parent.index).getMedia().outs.at(mercElementOutIndex) };
            std::visit(OutSetVideoBusData{ parent.merc, videoBusData, doubleBufferIndex }, out);
        }
        void operator()(const std::shared_ptr<Revolver<Pixels>>& revolver) const
        {
            auto& bullet{ revolver->lock() };
            videoBusData.texture = bullet.texture.get();
        }
    };

    struct InSetVideoBusData
    {
        const Merc& merc;
        vst::VideoBusData& videoBusData;
        uint64_t doubleBufferIndex{ 0 };
        void operator()(const Pixels* pixels) const
        {
            OutSetVideoBusData{ merc, videoBusData, doubleBufferIndex }(*pixels);
        }
        void operator()(const std::array<Pixels, 2>* doubleBuffer) const
        {
            OutSetVideoBusData{ merc, videoBusData, doubleBufferIndex }(*doubleBuffer);
        }
        void operator()(structure::UInt mercElementInIndex) const
        {
            const auto& parent{ merc.parent.value() };
            const auto& in{ parent.merc.getStepChains().at(parent.index).getMedia().ins.at(mercElementInIndex) };
            std::visit(InSetVideoBusData{ parent.merc, videoBusData, doubleBufferIndex }, in);
        }
    };

    template<typename T>
    struct FinalizeRun
    {
        const Merc& merc;
        void operator()(structure::UInt mercElementOutIndex) const
        {
            const auto& parent{ merc.parent.value() };
            const auto& out{ parent.merc. template getWork<Element<T>>().at(parent.index).getMedia().outs.at(mercElementOutIndex) };
            std::visit(FinalizeRun<T>{ parent.merc }, out);
        }
        void operator()(const std::shared_ptr<Revolver<typename ElementTraits<T>::Resource>>& revolver) const
        {
            revolver->load();
        }
        template<typename U>
        void operator()(const U& u) const
        {
        }
    };
}
