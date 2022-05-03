from conans import ConanFile, tools
from conans.errors import ConanException
from conan.tools.cmake import CMakeToolchain

class GerberaConan(ConanFile):
    name = "gerbera"
    license = "GPLv2"

    #generators = ("cmake", "cmake_find_package", "virtualrunenv")
    generators = ["CMakeDeps", "CMakeToolchain"]

    settings = "os", "arch", "compiler", "build_type"
    options = {
        "js": [True, False],
        "debug_logging": [True, False],
        "tests": [True, False],
        "magic": [True, False],
        "curl": [True, False],
        "taglib": [True, False],
        "exif": [True, False],
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
        "matroska": False,
        # The following are false in CMakeLists.txt, but almost always turned on.
        "mysql": True,
        "ffmpeg": True,
        "ffmpegthumbnailer": False,
    }

    scm = {"type": "git", "url": "auto", "revision": "auto"}

    requires = [
        "fmt/8.1.1",
        "spdlog/1.10.0",
        "pugixml/1.12.1",
        "libiconv/[>=1.16]",
        "sqlite3/[>=3.35.5]",
        "zlib/[>=1.2.12]",
        "pupnp/[>=1.14.6]",
        "taglib/1.12",
        "libuuid/[>=1.0.3]"
    ]

    def configure(self):
        tools.check_min_cppstd(self, "17")

        self.options["pupnp"].reuseaddr = True
        self.options["pupnp"].ipv6 = True
        self.options["pupnp"].largefile = True
        self.options["pupnp"].tools = True

        if self.options.tests:
            self.options["gtest"].no_main = False
            self.options["gtest"].shared = False
            self.options["gtest"].build_gmock = True

    @property
    def _needs_uuid(self):
        os_info = tools.OSInfo()
        return os_info.is_linux or os_info.is_macos

    def requirements(self):
        if self.options.tests:
            self.requires("gtest/1.11.0")

        if self.options.js:
            self.requires("duktape/[>=2.7.0]")

        if self.options.curl:
            self.requires("libcurl/[>=7.74.0]")

        if self.options.mysql:
            self.requires("mariadb-connector-c/[>=3.1.11]")

        if self.options.ffmpeg:
            self.requires("ffmpeg/[>=4.2.1]")

        if self.options.exif:
            self.requires("libexif/[>=0.6.23]")

    def system_requirements(self):
        if tools.cross_building(self):
            self.output.info("Cross-compiling, not installing system packages")
            return

        os_info = tools.OSInfo()
        if os_info.with_apt:
            pm = "apt"
        elif os_info.with_pacman:
            pm = "pacman"
        elif os_info.with_yum:
            pm = "yum"
        elif os_info.is_freebsd:
            pm = "freebsd"
        else:
            self.output.warn("Don't know how to install packages.")
            return

        installer = tools.SystemPackageTool(conanfile=self)
        if self.options.magic:
            installer.install(
                {
                    "apt": "libmagic-dev",
                    "pacman": "file",
                    "yum": "file-devel",
                    "freebsd": [],
                }[pm]
            )

        if self.options.matroska:
            installer.install(
                {
                    "apt": "libmatroska-dev",
                    "pacman": "libmatroska",
                    "yum": "libmatroska-devel",
                    "freebsd": "libmatroska",
                }[pm]
            )

        if self.options.ffmpegthumbnailer:
            installer.install(
                {
                    "apt": "libffmpegthumbnailer-dev",
                    "pacman": "ffmpegthumbnailer",
                    "yum": "ffmpegthumbnailer-devel",
                    "freebsd": "ffmpegthumbnailer",
                }[pm]
            )

    def generate(self):
        tc = CMakeToolchain(self)
        tc.variables["WITH_JS"] = self.options.js
        tc.variables["WITH_DEBUG"] = self.options.debug_logging
        tc.variables["WITH_TESTS"] = self.options.tests
        tc.variables["WITH_MAGIC"] = self.options.magic
        tc.variables["WITH_CURL"] = self.options.curl
        tc.variables["WITH_TAGLIB"] = self.options.taglib
        tc.variables["WITH_EXIF"] = self.options.exif
        tc.variables["WITH_MATROSKA"] = self.options.matroska
        tc.variables["WITH_MYSQL"] = self.options.mysql
        tc.variables["WITH_AVCODEC"] = self.options.ffmpeg
        tc.variables["WITH_FFMPEGTHUMBNAILER"] = self.options.ffmpegthumbnailer

        if self.settings.os != "Linux" or tools.cross_building(self):
            tc.variables["WITH_SYSTEMD"] = False

        tc.generate()
