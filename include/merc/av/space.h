#pragma once

#include <vector>
#include <filesystem>
#include <optional>
#include <string>
#include <unordered_map>
#include <set>
#include <unordered_set>
#include <pluginterfaces/vst/ivstcomponent.h>
#include <pluginterfaces/vst/ivsteditcontroller.h>
#include <public.sdk/source/vst/hosting/module.h>

namespace merc::av
{
    struct Space
    {
        Space();
        Steinberg::IPtr<Steinberg::Vst::IComponent> getComponent(const std::string& cidString);
        Steinberg::IPtr<Steinberg::Vst::IEditController> getEditor(const VST3::UID& editorID);
    private:
        struct Entry
        {
            std::string name;
            std::string category;
            std::string subCategories;
            std::string modulePath;
        };
        std::unordered_map<std::string, Entry> entries;
        std::unordered_map<std::string, VST3::Hosting::Module::Ptr> loadedModules;
        bool fromCache(const std::filesystem::path& cachePath,
                       const std::set<std::filesystem::path>& dirs);
        void fromDirs(const std::set<std::filesystem::path>& dirs,
                      const std::filesystem::path& cachePath,
                      const std::unordered_set<std::string>& modulesToIngore = {});
        void addClassesOrRecurse(const std::filesystem::path& dir,
                                 const std::unordered_set<std::string>& modulesToIgnore);
        void addClasses(const std::filesystem::path& modulePath);
        void writeCache(const std::filesystem::path& cachePath) const;
        static long long getLmt(const std::filesystem::path& path);
        static bool isInDirs(const std::filesystem::path& path, const std::set<std::filesystem::path>& dirs);
        VST3::Hosting::Module::Ptr& getModule(const std::string& cidString);
    };
}
