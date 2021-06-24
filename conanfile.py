from conans import CMake, ConanFile, tools
from conans.errors import ConanException


class GerberaConan(ConanFile):
    name = "gerbera"
    license = "GPLv2"

    generators = ("cmake", "cmake_find_package", "virtualrunenv")
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
        "matroska": True,
        # The following are false in CMakeLists.txt, but almost always turned on.
        "mysql": True,
        "ffmpeg": True,
        "ffmpegthumbnailer": True,
    }

    scm = {"type": "git", "url": "auto", "revision": "auto"}

    requires = [
        "fmt/[<8.0.0]",
        "spdlog/[>=1.8.5]",
        "pugixml/1.10",
        "libiconv/1.16",
        "sqlite3/[>=3.35.5]",
        "zlib/1.2.11",
        "pupnp/[>=1.14.0]",
    ]

    def configure(self):
        tools.check_min_cppstd(self, "17")
        if self.options.tests:
            # We have our own main function,
            # Moreover, if "shared" is True then main is an .so...
            self.options["gtest"].no_main = True

    @property
    def _needs_system_uuid(self):
        if self.options.ffmpeg:
            os_info = tools.OSInfo()
            # ffmpeg on Ubuntu has libuuid as a deep transitive dependency
            # and fails to link otherwise.
            return os_info.with_apt

    def requirements(self):
        if self.options.tests:
            self.requires("gtest/1.10.0")

        if self.options.js:
            self.requires("duktape/2.5.0")

        if self.options.curl:
            self.requires("libcurl/[>=7.74.0]")

        if self.options.mysql:
            self.requires("mariadb-connector-c/3.1.11")

        if not self._needs_system_uuid:
            self.requires("libuuid/1.0.3")

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

        if self.options.taglib:
            installer.install(
                {
                    "apt": "libtag1-dev",
                    "pacman": "taglib",
                    "yum": "libtag-devel",
                    "freebsd": "taglib",
                }[pm]
            )

        if self.options.exif:
            installer.install(
                {
                    "apt": "libexif-dev",
                    "pacman": "libexif",
                    "yum": "libexif-devel",
                    "freebsd": "libexif",
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

        if self.options.ffmpeg:
            installer.install(
                {
                    "apt": "libavformat-dev",
                    "pacman": "ffmpeg",
                    "yum": "ffmpeg-devel",
                    "freebsd": "ffmpeg",
                }[pm]
            )

        if self._needs_system_uuid:
            installer.install(
                {"apt": "uuid-dev", "pacman": [], "yum": [], "freebsd": []}[pm]
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

    def build(self):
        cmake = CMake(self)
        cmake.definitions["WITH_JS"] = self.options.js
        cmake.definitions["WITH_DEBUG"] = self.options.debug_logging
        cmake.definitions["WITH_TESTS"] = self.options.tests
        cmake.definitions["WITH_MAGIC"] = self.options.magic
        cmake.definitions["WITH_CURL"] = self.options.curl
        cmake.definitions["WITH_TAGLIB"] = self.options.taglib
        cmake.definitions["WITH_EXIF"] = self.options.exif
        cmake.definitions["WITH_MATROSKA"] = self.options.matroska
        cmake.definitions["WITH_MYSQL"] = self.options.mysql
        cmake.definitions["WITH_AVCODEC"] = self.options.ffmpeg
        cmake.definitions["WITH_FFMPEGTHUMBNAILER"] = self.options.ffmpegthumbnailer

        if self.settings.os != "Linux" or tools.cross_building(self):
            cmake.definitions["WITH_SYSTEMD"] = False

        cmake.configure()
        cmake.build()

        if tools.get_env("CONAN_RUN_TESTS", True):
            with tools.run_environment(self):
                cmake.test(output_on_failure=True)
