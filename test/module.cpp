#include <pluginterfaces/vst/ivstaudioprocessor.h>
#include <pluginterfaces/vst/ivsthostapplication.h>
#include <base/source/fobject.h>
#include <public.sdk/source/vst/hosting/module.h>
#include <gtest/gtest.h>
#include <algorithm>

struct SomeHostApplication : Steinberg::FObject, Steinberg::Vst::IHostApplication
{
    Steinberg::tresult PLUGIN_API getName(Steinberg::Vst::String128 name) override
    {
        static const auto hostName{ L"SomeHost" };
        std::copy(hostName, hostName + 9, name);
        return Steinberg::kResultOk;
    }
    Steinberg::tresult PLUGIN_API createInstance(Steinberg::TUID cid, Steinberg::TUID _iid, void** obj) override
    {
        return Steinberg::kNotImplemented;
    }
    OBJ_METHODS(SomeHostApplication, Steinberg::FObject)
    DEFINE_INTERFACES
        DEF_INTERFACE(Steinberg::Vst::IHostApplication)
    END_DEFINE_INTERFACES(Steinberg::FObject)
    REFCOUNT_METHODS(Steinberg::FObject)
};

TEST(MercModule, CanLoadBuzz)
{
    std::string error;
    auto buzz{ VST3::Hosting::Module::create(TEST_CMAKE_BINARY_DIR "/vst3/buzz.vst3", error) };
    ASSERT_EQ(error, "");
    ASSERT_TRUE(buzz);
    const auto processorUid{ buzz->getFactory().classInfos()[0].ID() };
    auto component{ buzz->getFactory().createInstance<Steinberg::Vst::IComponent>(processorUid) };
    Steinberg::FUnknownPtr<Steinberg::Vst::IAudioProcessor> processor{ component };
    ASSERT_TRUE(processor);
    ASSERT_TRUE(component);
    SomeHostApplication host;
    component->initialize((Steinberg::FUnknown*)(void*)&host);
    Steinberg::Vst::ProcessSetup setup
    {
        Steinberg::Vst::kRealtime,
        Steinberg::Vst::kSample32,
        512,
        44100
    };
    ASSERT_EQ(processor->setupProcessing(setup), Steinberg::kResultOk);
    ASSERT_EQ(component->activateBus(Steinberg::Vst::kAudio, Steinberg::Vst::kOutput, 0, true), Steinberg::kResultOk);
    ASSERT_EQ(component->setActive(true), Steinberg::kResultOk);
    ASSERT_EQ(processor->setProcessing(true), Steinberg::kResultOk);
    ASSERT_EQ(component->terminate(), Steinberg::kResultOk);
}
