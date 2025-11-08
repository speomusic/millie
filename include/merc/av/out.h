#pragma once

#include <memory>
#include "merc/av/resource.h"
#include "merc/av/revolver.h"
#include "merc/vst/factory.h"

namespace merc::av
{
    using SampleRevolver = std::shared_ptr<Revolver<Samples>>;
    using PixelRevolver = std::shared_ptr<Revolver<Pixels>>;
    struct Out
    {
        Out(const vst::Factory& factory);
        std::tuple<SampleRevolver, PixelRevolver> revolvers;
    };
}
