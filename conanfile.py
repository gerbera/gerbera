import os
from conan import ConanFile
from conan.tools.files import save, load, copy, get
from conan.tools.build import check_min_cppstd, cross_building
from conan.tools.cmake import CMakeToolchain, CMakeDeps, CMake, cmake_layout
from conan.tools.env import Environment
from conan.tools.system.package_manager import Apt, Yum, PacMan, Zypper, Pkg
from conan.tools.gnu import AutotoolsToolchain, AutotoolsDeps, Autotools, PkgConfigDeps, PkgConfig
from conan.errors import ConanInvalidConfiguration
from conan.errors import ConanException

class GerberaConan(ConanFile):
    name = "gerbera"
    license = "GPLv2"
    settings = "os", "arch", "compiler", "build_type"
    exports_sources = "CMakeLists.txt", "cmake/*", "src/*", "test/*", "scripts/systemd/gerbera.service.cmake", "scripts/js/*"

    options = {
        "js": [True, False],
        "debug_logging": [True, False],
        "tests": [True, False],
        "magic": [True, False],
        "curl": [True, False],
        "taglib": [True, False],
        "exif": [True, False],
        "exiv2": [True, False],
        "matroska": [True, False],
        "mysql": [True, False],
        "ffmpeg": [True, False],
        "ffmpegthumbnailer": [True, False],
    }
    default_options = {
        "js": True,
        "debug_logging": False,
        "tests": False,
        "magic": True,
        "curl": True,
        "taglib": True,
        "exif": True,
        # The following are false in CMakeLists.txt, but almost always turned on.
        "exiv2": False,
        "matroska": True,
        "mysql": True,
        "ffmpeg": True,
        "ffmpegthumbnailer": True,
    }

    scm = {"type": "git", "url": "auto", "revision": "auto"}

    requires = [
        "fmt/10.2.1", # spdlog/1.14.1 set this version
        "spdlog/1.14.1",
        "pugixml/1.14",
        "libiconv/[>=1.16]",
        "sqlite3/[>=3.35.5]",
        "zlib/1.3.1",
        "pupnp/[>=1.14.16]",
        "taglib/[>=1.12]",
        "openssl/1.1.1w", # Needed to fix conflicts
    ]

    def layout(self):
        cmake_layout(self, src_folder=".")

    def configure(self):
        check_min_cppstd(self, "17")
        if self.options.tests:
            # We have our own main function,
            # Moreover, if "shared" is True then main is an .so...
            self.options["gtest"].no_main = True

    @property
    def _needs_system_uuid(self):
        if self.options.ffmpeg:
            # ffmpeg on Ubuntu has libuuid as a deep transitive dependency
            # and fails to link otherwise.
            return Apt(self).check(["libuuid/1.0.3"])

    def requirements(self):
        if self.options.tests:
            self.requires("gtest/[>=1.13.0]")

        if self.options.js:
            self.requires("duktape/[>=2.7.0]")

        if self.options.curl:
            self.requires("libcurl/[>=7.85.0]")

        if self.options.exiv2:
            self.requires("inih/57")
            self.requires("exiv2/0.28.1")

        if self.options.mysql:
            self.requires("mariadb-connector-c/[<3.4.3]") # That recipe is broken

        if self._needs_system_uuid:
            self.requires("libuuid/1.0.3")

    def system_requirements(self):
        if cross_building(self):
            self.output.info("Cross-compiling, not installing system packages")
            return

        if self.options.magic:
            Apt(self).install(["libmagic-dev"])
            PacMan(self).install(["file"])
            Zypper(self).install(["file"])
            Yum(self).install(["file-devel"])

        if self.options.exif:
            Apt(self).install(["libexif-dev"])
            PacMan(self).install(["libexif"])
            Zypper(self).install(["libexif12"])
            Yum(self).install(["libexif-devel"])
            Pkg(self).install(["libexif"])

        if self.options.matroska:
            Apt(self).install(["libmatroska-dev"])
            PacMan(self).install(["libmatroska"])
            Zypper(self).install(["libmatroska-devel"])
            Yum(self).install(["libmatroska-devel"])
            Pkg(self).install(["libmatroska"])

        if self.options.ffmpeg:
            Apt(self).install(["libavformat-dev"])
            PacMan(self).install(["ffmpeg"])
            Zypper(self).install(["ffmpeg-4"])
            Yum(self).install(["ffmpeg-devel"])
            Pkg(self).install(["ffmpeg"])

        Apt(self).install(["uuid-dev"]);

        if self.options.ffmpegthumbnailer:
            Apt(self).install(["libffmpegthumbnailer-dev"])
            PacMan(self).install(["ffmpegthumbnailer"])
            Zypper(self).install(["ffmpegthumbnailer"])
            Yum(self).install(["ffmpegthumbnailer-devel"])
            Pkg(self).install(["ffmpegthumbnailer"])

    def generate(self):
        tc = CMakeToolchain(self)
        tc.cache_variables["WITH_JS"] = self.options.js
        tc.cache_variables["WITH_DEBUG"] = self.options.debug_logging
        tc.cache_variables["WITH_TESTS"] = self.options.tests
        tc.cache_variables["WITH_MAGIC"] = self.options.magic
        tc.cache_variables["WITH_CURL"] = self.options.curl
        tc.cache_variables["WITH_TAGLIB"] = self.options.taglib
        tc.cache_variables["WITH_EXIF"] = self.options.exif
        tc.cache_variables["WITH_EXIV2"] = self.options.exiv2
        tc.cache_variables["WITH_MATROSKA"] = self.options.matroska
        tc.cache_variables["WITH_MYSQL"] = self.options.mysql
        tc.cache_variables["WITH_AVCODEC"] = self.options.ffmpeg
        tc.cache_variables["WITH_FFMPEGTHUMBNAILER"] = self.options.ffmpegthumbnailer

        if self.settings.os != "Linux" or cross_building(self):
            tc.variables["WITH_SYSTEMD"] = False

        tc.variables["CMAKE_FIND_PACKAGE_SORT_ORDER"] = "NATURAL"
        tc.variables["CMAKE_FIND_PACKAGE_PREFER_CONFIG"] = "OFF"
        tc.generate()
        deps = CMakeDeps(self)
        deps.set_property("fmt", "cmake_config_version_compat", "AnyNewerVersion")
        deps.generate()

    def build(self):
        copy(self, "CMakeLists.txt", src=self.recipe_folder, dst=self.build_folder)
        copy(self, "cmake/*.cmake", self.recipe_folder, self.build_folder)
        copy(self, "test", self.recipe_folder, self.build_folder)
        copy(self, "src", self.recipe_folder, self.build_folder)
        copy(self, "scripts/systemd/gerbera.service.cmake", self.recipe_folder, self.build_folder)
        copy(self, "scripts/js", self.recipe_folder, self.build_folder)
        cmake = CMake(self)
        # cmake_args = "--fresh --debug-find" # or could be "--debug-output"
        # cmake_args = "--debug-find" # or could be "--debug-output"
        #cmake.configure(cli_args=[cmake_args])
        cmake.configure()
        cmake.build()

        env = Environment()
        envvars = env.vars(self, scope="run")
        if envvars.get("CONAN_RUN_TESTS", True):
            env.define("CTEST_OUTPUT_ON_FAILURE", "1")
            with envvars.apply():
                cmake_args = "ARGS=\"--output-on-failure\""
                cmake.test(cli_args=[cmake_args])

