#include <gtest/gtest.h>
#include "merc/global/config.h"
#include "merc/av/space.h"
#include "util.h"

TEST(MercSpace, CanMakeFromFoldersWithCache)
{
    auto& config{ merc::global::loadConfig(getSpaceTestConfigPath()) };
    config.vstDirs.push_back(getTestVstDir());
    deleteExisting(merc::global::getConfigPath().parent_path() / "merc-space-cache.csv");
    {
        merc::av::Space space{};
        auto buzz{ space.getComponent("8F035F10DE6A3FC5A5A761DC12E2BDEF") };
        ASSERT_TRUE(buzz);
    }
    {
        merc::av::Space space{};
        auto buzz{ space.getComponent("8F035F10DE6A3FC5A5A761DC12E2BDEF") };
        ASSERT_TRUE(buzz);
    }
}
