#pragma once

#include "merc/av/merc-av.h"
#include "merc/av/out.h"
#include "merc/av/worker.h"

namespace merc::av
{
    using Winches = Workers<Link>;
    using Pipelines = Workers<Step>;

    struct Mercer
    {
        Mercer(Merc& merc, Out& outsToUnload);
        ~Mercer();

        Merc& merc;
    private:
        std::tuple<Winches, Pipelines> workers;
        Out& outsToUnload;
    };
}
