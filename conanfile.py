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

    scm = {"type": "git"}

    requires = [
        "fmt/6.2.1",
        "spdlog/1.6.0",
        "pugixml/1.10",
        "libiconv/1.16",
        "sqlite3/3.31.1",
        "zlib/1.2.11",
        "pupnp/1.12.1",
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

        if self.options.js:
            self.requires("duktape/2.5.0")

        if not self.options.ffmpeg:
            # ffmpeg has libuuid as a deep transitive dependency
            # and fails to link otherwise.
            self.requires("libuuid/1.0.3")

    def system_requirements(self):
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
                    "pacman": "file-dev",
                    "yum": "file-devel",
                    "freebsd": [],
                }[pm]
            )

        if self.options.taglib:
            installer.install(
                {
                    "apt": "libtag1-dev",
                    "pacman": "taglib-dev",
                    "yum": "libtag-devel",
                    "freebsd": "taglib",
                }[pm]
            )

        if self.options.exif:
            installer.install(
                {
                    "apt": "libexif-dev",
                    "pacman": "libexif-dev",
                    "yum": "libexif-devel",
                    "freebsd": "libexif",
                }[pm]
            )

        if self.options.matroska:
            installer.install(
                {
                    "apt": "libmatroska-dev",
                    "pacman": "libmatroska-dev",
                    "yum": "libmatroska-devel",
                    "freebsd": "libmatroska",
                }[pm]
            )

        # Note: there is a CURL Conan package, but it depends on openssl
        # which is also in Conan.
        if self.options.curl:
            installer.install(
                {
                    "apt": "libcurl4-openssl-dev",
                    "pacman": "curl-dev",
                    "yum": "libcurl-devel",
                    "freebsd": "curl",
                }[pm]
            )

        if self.options.mysql:
            installer.install(
                {
                    "apt": "libmariadb-dev",
                    "pacman": "mariadb-connector-c-dev",
                    "yum": "mariadb-connector-c-devel",
                    "freebsd": "mysql-connector-c",
                }[pm]
            )

        if self.options.ffmpeg:
            installer.install(
                {
                    "apt": "libavformat-dev",
                    "pacman": "ffmpeg-dev",
                    "yum": "ffmpeg-devel",
                    "freebsd": "ffmpeg",
                }[pm]
            )

        if self.options.ffmpegthumbnailer:
            installer.install(
                {
                    "apt": "libffmpegthumbnailer-dev",
                    "pacman": "ffmpegthumbnailer-dev",
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

        if self.settings.os != "Linux":
            cmake.definitions["WITH_SYSTEMD"] = False

        cmake.configure()
        cmake.build()
