#import <Metal/Metal.h>
#import "merc/av/resource.h"
#import "merc/vst/graphics.h"

namespace merc::av
{
    Pixels::Pixels(const vst::TextureBusArrangement& arrangement, const vst::Graphics& graphics)
        : texture{ std::make_unique<vst::Texture>(arrangement, graphics.device) }
    {}
    Pixels::Pixels(const Pixels& other)
        : texture{ std::make_unique<vst::Texture>(*other.texture) }
    {}
    Pixels::Pixels(Pixels&& other) = default;
    Pixels& Pixels::operator=(Pixels&& other) = default;
    Pixels::~Pixels() = default;
}
