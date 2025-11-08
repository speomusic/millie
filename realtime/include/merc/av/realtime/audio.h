#pragma once

#include <memory>
#include "merc/av/out.h"

namespace merc::av::realtime
{
    struct Audio
    {
        Audio(const Out& mainOut);
        ~Audio();
    private:
        std::unique_ptr<struct AudioImplementation> implementation;
    };
}
