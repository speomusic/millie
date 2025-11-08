#include <sstream>
#include <fstream>
#include <algorithm>
#include <public.sdk/source/vst/moduleinfo/moduleinfoparser.h>
#include <public.sdk/source/common/readfile.h>
#include "merc/av/space.h"
#include "merc/global/paths.h"
#include "merc/global/config.h"

namespace merc::av
{
    static const char* const cacheHeader{ "cid,name,category,subCategories,module,lmt" };

    static std::string makeSubCategoriesString(const std::vector<std::string>& subCategories)
    {
        if (subCategories.empty()) return {};
        std::string r; r.reserve(64);
        for (const auto& c : subCategories) r += "|" + c;
        r += "|";
        return r;
    }

    Space::Space()
    {
        std::set<std::filesystem::path> canonicals;
        canonicals.insert(std::filesystem::canonical(global::getVstDefaultUserDir()));
        canonicals.insert(std::filesystem::canonical(global::getVstDefaultSystemDir()));
        for (const auto& d : global::getConfig().vstDirs)
            if (std::filesystem::is_directory(d))
                canonicals.insert(std::filesystem::canonical(d));
        auto cachePath{ global::getConfigPath().parent_path() / global::getConfig().spaceCacheDir / "merc-space-cache.csv" };
        if (!std::filesystem::exists(cachePath)
            || !fromCache(cachePath, canonicals))
            fromDirs(canonicals, cachePath);
    }

    Steinberg::IPtr<Steinberg::Vst::IComponent> Space::getComponent(const std::string& cidString)
    {
        if (const auto cid{ VST3::UID::fromString(cidString) })
            return getModule(cidString)->getFactory().createInstance<Steinberg::Vst::IComponent>(cid.value());
        throw std::runtime_error("bad UID string: " + cidString);
    }

    Steinberg::IPtr<Steinberg::Vst::IEditController> Space::getEditor(const VST3::UID& editorID)
    {
        return getModule(editorID.toString())->getFactory().createInstance<Steinberg::Vst::IEditController>(editorID);
    }

    VST3::Hosting::Module::Ptr& Space::getModule(const std::string& cidString)
    {
        const auto& entry{ entries.at(cidString) };
        auto& mod{ loadedModules[entry.modulePath] };
        if (!mod)
        {
            std::string error;
            mod = VST3::Hosting::Module::create(entry.modulePath, error);
            if (!error.empty()) throw std::runtime_error(error);
        }
        return mod;
    }

    bool Space::fromCache(const std::filesystem::path& cachePath,
                          const std::set<std::filesystem::path>& dirs)
    {
        std::unordered_set<std::string> modulesReadFromCache;
        std::ifstream cache{ cachePath };
        std::string line, value;
        std::getline(cache, line);
        if (line != cacheHeader) throw std::runtime_error("bad merc::Space cache");
        while (std::getline(cache, line))
        {
            std::istringstream s{ line };
            std::getline(s, value, ',');
            auto& entry{ entries[value] };
            std::getline(s, entry.name, ',');
            std::getline(s, entry.category, ',');
            std::getline(s, entry.subCategories, ',');
            std::getline(s, entry.modulePath, ',');
            std::filesystem::path modulePath{ entry.modulePath };
            if (!std::filesystem::is_directory(modulePath)
                || modulePath.extension() != ".vst3"
                || !isInDirs(modulePath, dirs))
            {
                entries.clear();
                return false;
            }
            long long lmt;
            s >> lmt;
            if (lmt < getLmt(entry.modulePath))
            {
                entries.clear();
                return false;
            }
            modulesReadFromCache.insert(entry.modulePath);
        }
        fromDirs(dirs, cachePath, modulesReadFromCache);
        return true;
    }

    void Space::fromDirs(const std::set<std::filesystem::path>& dirs,
                         const std::filesystem::path& cachePath,
                         const std::unordered_set<std::string>& modulesToIgnore)
    {
        const auto numEntriesBefore{ entries.size() };
        for (const auto& dir : dirs)
            if (std::filesystem::is_directory(dir))
                addClassesOrRecurse(dir, modulesToIgnore);
        if (numEntriesBefore != entries.size())
            writeCache(cachePath);
    }

    void Space::addClassesOrRecurse(const std::filesystem::path& dir,
                                    const std::unordered_set<std::string>& modulesToIgnore)
    {
        if (dir.extension() == ".vst3"
            && !modulesToIgnore.contains(dir.string()))
            addClasses(dir);
        else
            for (const auto& entry : std::filesystem::directory_iterator{ dir })
                if (entry.is_directory())
                    addClassesOrRecurse(entry.path(), modulesToIgnore);
    }

    void Space::addClasses(const std::filesystem::path& modulePath)
    {
        if (const auto infoPath{ VST3::Hosting::Module::getModuleInfoPath(modulePath.string()) })
        {
            if (const auto info{ Steinberg::ModuleInfoLib::parseJson(Steinberg::readFile(infoPath.value()), nullptr) })
            {
                for (const auto& c : info.value().classes)
                    entries.insert({ c.cid, { c.name, c.category, makeSubCategoriesString(c.subCategories), modulePath.string() } });
                return;
            }
        }
        std::string error;
        auto m{ VST3::Hosting::Module::create(modulePath.string(), error)};
        if (!error.empty()) throw std::runtime_error(error);
        for (const auto& c : m->getFactory().classInfos())
            entries.insert({ c.ID().toString(), { c.name(), c.category(), makeSubCategoriesString(c.subCategories()), modulePath.string() } });
    }

    void Space::writeCache(const std::filesystem::path& cachePath) const
    {
        std::ofstream out{ cachePath };
        out << cacheHeader << "\n";
        for (const auto& [cid, entry] : entries)
            out << cid << "," << entry.name << "," << entry.category << "," << entry.subCategories << "," << entry.modulePath << "," << getLmt(entry.modulePath) << "\n";
    }

    long long Space::getLmt(const std::filesystem::path& path)
    {
        const auto lmtDuration{ std::filesystem::last_write_time(path).time_since_epoch() };
        return std::chrono::duration_cast<std::chrono::milliseconds>(lmtDuration).count();
    }

    bool Space::isInDirs(const std::filesystem::path& path, const std::set<std::filesystem::path>& dirs)
    {
        for (const auto& dir : dirs)
            if (std::mismatch(path.begin(), path.end(), dir.begin()).second == dir.end())
                return true;
        return false;
    }
}
