#include <gtest/gtest.h>
#include "merc/av/space.h"
#include "merc/global/config.h"
#include "merc/merc.h"
#include "merc/vst/factory.h"
#include "util.h"

TEST(Merc, CanSaveBerserc)
{
    deleteExistingMerc("sine.merc");
    deleteExistingMerc("triangle.merc");
    deleteExistingMerc("berserc.merc");

    merc::global::loadConfig(getBersercConfigPath()).mercStorageDir = getMercStorageDir();
    merc::av::Space space; 
    merc::vst::Factory factory;

    merc::Merc sineMerc{ "sine.merc", space, factory };
    {
        merc::structure::Link buzz{ merc::structure::VstRunner{ "8F035F10DE6A3FC5A5A761DC12E2BDEF" } };
        buzz.routing.outs.push_back(merc::structure::MercOut{});
        auto transaction{ sineMerc.database.openReadWriteTransaction(merc::interface::MercIndex{}) };
        transaction.writable.linkChains.try_emplace("Main", merc::structure::LinkChain{ std::move(buzz) });
        ASSERT_TRUE(transaction.commit());
    }

    merc::Merc triangleMerc{ "triangle.merc", space, factory };
    {
        merc::structure::Step triangle{ merc::structure::VstRunner{ "CCF29ACC1C1A332899C0953DF892B244" } };
        triangle.routing.outs.push_back(merc::structure::MercOut{});
        auto transaction{ triangleMerc.database.openReadWriteTransaction(merc::interface::MercIndex{}) };
        transaction.writable.stepChains.try_emplace("Main", merc::structure::StepChain{ std::move(triangle) });
        transaction.writable.eventRouting.try_emplace(0, merc::structure::Indexes{ { "Main", 0 } });
        ASSERT_TRUE(transaction.commit());
    }

    merc::Merc bersercMerc{ "berserc.merc", space, factory };
    {
        auto sineTransaction{ sineMerc.database.openReadTransaction(merc::interface::MercIndex{}) };
        auto sineMerc{ sineTransaction.readable };
        ASSERT_TRUE(sineTransaction.commit());

        auto triangleTransaction{ triangleMerc.database.openReadTransaction(merc::interface::MercIndex{}) };
        auto triangleMerc{ triangleTransaction.readable };
        ASSERT_TRUE(triangleTransaction.commit());

        merc::structure::Link sine{ merc::structure::ChildMercRunner{} };
        sine.routing.outs.push_back(merc::structure::MainOut{});
        merc::structure::Step triangle{ merc::structure::ChildMercRunner{} };
        triangle.routing.outs.push_back(merc::structure::MainOut{});

        auto transaction{ bersercMerc.database.openReadWriteTransaction(merc::interface::MercIndex{}) };
        transaction.writable.linkChains.try_emplace("Main", merc::structure::LinkChain{ std::move(sine) });
        transaction.writable.stepChains.try_emplace("Main", merc::structure::StepChain{ std::move(triangle) });
        transaction.writable.audioMercs.try_emplace(merc::structure::Index{ "Main", 0 }, std::move(sineMerc));
        transaction.writable.videoMercs.try_emplace(merc::structure::Index{ "Main", 0 }, std::move(triangleMerc));
        ASSERT_TRUE(transaction.commit());
    }
}

