#import <AudioToolbox/AudioToolbox.h>
#import "merc/av/realtime/audio.h"
#import "merc/global/config.h"

namespace merc::av::realtime
{
    static OSStatus renderCallback(void* refCon,
                                   AudioUnitRenderActionFlags* ioActionFlags,
                                   const AudioTimeStamp* inTimeStamp,
                                   UInt32 inBusNumber,
                                   UInt32 inNumberFrames,
                                   AudioBufferList* ioData);

    struct AudioImplementation
    {
        AudioImplementation(std::shared_ptr<Revolver<Samples>> r, unsigned sampleRate)
            : revolver{ std::move(r) }
            , buffer{ revolver->inspect() }
        {
            AudioComponentDescription defaultOutputDescription
            {
                .componentType = kAudioUnitType_Output,
                .componentSubType = kAudioUnitSubType_DefaultOutput,
                .componentManufacturer = kAudioUnitManufacturer_Apple
            };
            AudioComponent defaultOutput = AudioComponentFindNext(NULL, &defaultOutputDescription);
            if (!defaultOutput) throw std::runtime_error("Could not find default output audio unit component");

            auto err{ AudioComponentInstanceNew(defaultOutput, &outputAudioUnit) };
            if (err != noErr) throw std::runtime_error("Could not create default output audio unit");

            AudioStreamBasicDescription streamFormat
            {
                .mSampleRate = static_cast<double>(sampleRate),
                .mFormatID = kAudioFormatLinearPCM,
                .mFormatFlags = kAudioFormatFlagIsFloat,
                .mBytesPerPacket = 2 * sizeof(float),
                .mFramesPerPacket = 1,
                .mBytesPerFrame = 2 * sizeof(float),
                .mChannelsPerFrame = 2,
                .mBitsPerChannel = sizeof(float) * 8
            };
            err = AudioUnitSetProperty(outputAudioUnit,
                                       kAudioUnitProperty_StreamFormat,
                                       kAudioUnitScope_Input,
                                       0,
                                       &streamFormat,
                                       sizeof(AudioStreamBasicDescription));
            if (err != noErr) throw std::runtime_error("Could not set input format of default output audio unit");

            AURenderCallbackStruct input
            {
                .inputProc = renderCallback,
                .inputProcRefCon = this
            };
            err = AudioUnitSetProperty(outputAudioUnit,
                                       kAudioUnitProperty_SetRenderCallback,
                                       kAudioUnitScope_Input,
                                       0,
                                       &input,
                                       sizeof(input));
            if (err != noErr) throw std::runtime_error("Could not set render callback on default output audio unit");

            err = AudioUnitInitialize(outputAudioUnit);
            if (err != noErr) throw std::runtime_error("Could not initialize default output audio unit");

            err = AudioOutputUnitStart(outputAudioUnit);
            if (err != noErr) throw std::runtime_error("Could not start default output audio unit");
        }

        ~AudioImplementation()
        {
            AudioOutputUnitStop(outputAudioUnit);
        }

        AudioComponentInstance outputAudioUnit;
        std::shared_ptr<Revolver<Samples>> revolver;
        Samples buffer;
        size_t framesLeftInBuffer{ 0 };
    };

    void putFrame(float* interleavedDst, size_t fDst, const Samples& src, size_t fSrc)
    {
        *(interleavedDst + 0 + fDst * 2) = src.channels[0][fSrc];
        *(interleavedDst + 1 + fDst * 2) = src.channels[1][fSrc];
    }

    static OSStatus renderCallback(void* refCon,
                                   AudioUnitRenderActionFlags* ioActionFlags,
                                   const AudioTimeStamp* inTimeStamp,
                                   UInt32 inBusNumber,
                                   UInt32 inNumberFrames,
                                   AudioBufferList* ioData)
    {
        auto rt{ reinterpret_cast<AudioImplementation*>(refCon) };
        auto dataOut{ reinterpret_cast<float*>(ioData->mBuffers[0].mData) };
        int frameIndex{ 0 };
        while (frameIndex < inNumberFrames)
        {
            while (rt->framesLeftInBuffer > 0 && frameIndex < inNumberFrames)
                putFrame(dataOut,
                         frameIndex++,
                         rt->buffer,
                         rt->buffer.channels[0].size() - rt->framesLeftInBuffer--);
            if (frameIndex < inNumberFrames)
            {
                auto shot{ rt->revolver->fire() };
                if (!shot.blank) rt->buffer = *shot.bullet;
                else std::fill(rt->buffer.data.begin(), rt->buffer.data.end(), 0.0f);
                rt->framesLeftInBuffer = rt->buffer.channels[0].size();
            }
        }
        return 0;
    }

    Audio::Audio(const Out& mainOut)
        : implementation{ std::make_unique<AudioImplementation>(std::get<SampleRevolver>(mainOut.revolvers), global::getConfig().audio.sampleRate) }
    {}
    Audio::~Audio() {}
}
