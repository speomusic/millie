#pragma once

#include <filesystem>
#include <memory>

inline std::filesystem::path getMercStorageDir()
{
    return TEST_CMAKE_BINARY_DIR "/mercs";
}

inline std::filesystem::path getTestVstDir()
{
    return TEST_CMAKE_BINARY_DIR "/vst3";
}

inline std::filesystem::path getSpaceTestConfigPath()
{
    return TEST_CMAKE_BINARY_DIR "/space-test.conf";
}

inline std::filesystem::path getBersercConfigPath()
{
    return TEST_CMAKE_BINARY_DIR "/berserc.conf";
}

inline void deleteExisting(std::filesystem::path path)
{
    if (std::filesystem::is_regular_file(path))
        std::filesystem::remove(path);
}

inline void deleteExistingMerc(std::filesystem::path mercPath)
{
    deleteExisting(getMercStorageDir() / mercPath);
}
