from conans import ConanFile, CMake, tools


class GerberaConan(ConanFile):
    name = "gerbera"
    license = "GPLv2"

    generators = ("cmake", "cmake_find_package")
    settings = "os", "arch", "compiler", "build_type"
    options = {
        "js": [True, False],
        "debug_logging": [True, False],
        "tests": [True, False],
    }
    default_options = {
        "js": True,
        "debug_logging": False,
        "tests": False,
    }

    scm = {"type": "git"}

    requires = [
        "fmt/6.2.1",
        "spdlog/1.6.0",
        "pugixml/1.10@bincrafters/stable",
        "libuuid/1.0.3",
        "libiconv/1.16",
        "sqlite3/3.31.1",
        "zlib/1.2.11",
    ]

    def configure(self):
        tools.check_min_cppstd(self, "17")
        if self.options.tests:
            # We have our own main function,
            # Moreover, if "shared" is True then main is an .so...
            self.options["gtest"].no_main = True

    def requirements(self):
        if self.options.tests:
            self.requires("gtest/1.10.0")

    def build(self):
        cmake = CMake(self)
        cmake.definitions["WITH_JS"] = self.options.js
        cmake.definitions["WITH_DEBUG"] = self.options.debug_logging
        cmake.definitions["WITH_TESTS"] = self.options.tests
        cmake.configure()
        cmake.build()
