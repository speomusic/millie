#include <gtest/gtest.h>
#include "merc/global/config.h"
#include "util.h"

TEST(MercConfig, CanSaveBersercConfig)
{
    deleteExisting(getBersercConfigPath());
    auto& conf{ merc::global::loadConfig(getBersercConfigPath()) };
    conf.vstDirs = { getTestVstDir() };
    conf.mercStorageDir = getMercStorageDir();
    conf.audio.numWinches = 10;
    conf.video.numPipelines = 10;
    merc::global::saveConfig();
}

