#include "merc/av/mercer.h"

namespace merc::av
{
    Mercer::Mercer(Merc& m, Out& os)
        : merc{ m }
        , outsToUnload{ os }
    {
        std::get<Winches>(workers).start(global::getConfig().audio.numWinches, &merc);
        std::get<Pipelines>(workers).start(global::getConfig().video.numPipelines, &merc);
    }

    Mercer::~Mercer()
    {
        // workers might be waiting to load already-loaded revolvers in mercs
        // workers are signalled before unloading merc revolvers, so they know to stop right after loading the abandoned revolver
        std::get<Winches>(workers).cancel();
        std::get<Pipelines>(workers).cancel();

        std::get<SampleRevolver>(outsToUnload.revolvers)->unload();
        std::get<PixelRevolver>(outsToUnload.revolvers)->unload();

        std::get<Winches>(workers).clear();
        std::get<Pipelines>(workers).clear();
    }
}
