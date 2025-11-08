#include "merc/av/out.h"
#include "merc/av/vst-step.h"
#include "merc/global/config.h"

namespace merc::av
{
    Out::Out(const vst::Factory& factory)
        : revolvers
        {
            std::make_shared<Revolver<Samples>>(global::getConfig().audio.numChambers,
                                                Samples{ 2, global::getConfig().audio.batchSize }),
            std::make_shared<Revolver<Pixels>>(global::getConfig().video.numChambers,
                                               Pixels{ toVstVideoBusArrangement(getMainOutArrangement<structure::Step>()).textureBus, *factory.graphics })
        }
    {}
}
