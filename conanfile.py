from conan import ConanFile
from conan.tools.cmake import CMake, CMakeToolchain, CMakeDeps, cmake_layout
from conan.tools.scm import Git

class Millie(ConanFile):
    name = "millie"
    version = "1.0.0"
    author = "Jan jan@speomusic.com"
    url = "github.com/speomusic/millie.git"
    description = "Reference implementation of the Millie protocol."
    settings = "os", "compiler", "build_type", "arch"
    exports_sources = "include/*", "source/*", "test/*", "CMakeLists.txt"

    def source(self):
        Git(self).clone("https://github.com/steinbergmedia/vst3sdk.git")
        Git(self, "vst3sdk").checkout("v3.8.0_build_66")
        for submodule in ["base", "cmake", "pluginterfaces", "public.sdk"]:
            Git(self, "vst3sdk").run(f"submodule update --init {submodule}")

    def build_requirements(self):
        self.test_requires("gtest/1.14.0")

    def layout(self):
        cmake_layout(self)

    def generate(self):
        g = { "Macos": "Xcode", "Windows": "Visual Studio 17 2022" }[str(self.settings.os)]
        tc = CMakeToolchain(self, generator = g)
        tc.user_presets_path = "ConanPresets.json"
        tc.cache_variables = {
                "SMTG_ENABLE_VSTGUI_SUPPORT": "OFF",
                "SMTG_CREATE_PLUGIN_LINK": "OFF",
                "SMTG_ENABLE_VST3_HOSTING_EXAMPLES": "OFF",
                "SMTG_ENABLE_VST3_PLUGIN_EXAMPLES": "OFF",
                "SMTG_RUN_VST_VALIDATOR": "OFF"
                }
        tc.generate()
        CMakeDeps(self).generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.install()
