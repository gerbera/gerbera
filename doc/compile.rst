.. index:: Compile Gerbera

Compile Gerbera
===============

Gerbera uses the excellent `CMake <https://cmake.org/>`__ build system.

When gerbera is already installed, running ``gerbera --compile-info`` will show the compilation options used.


.. _gerbera-requirements:

Development Environment
~~~~~~~~~~~~~~~~~~~~~~~

Gerbera supports compiling with g++ (from version 9) and clang (from version 11).
You also have to install cmake (at least version 3.19), pkg-config and autoconf.

Dependencies
~~~~~~~~~~~~

.. Note:: Remember to install associated development packages, because development headers are needed for compilation!

In order to compile Gerbera you will have to install the following packages if the respective compile-time option is set to ``ON``.

Required Packages
-----------------

+------------+------------------------------+---------------------+--------------------+
| Library    | Note                         | Compile-time option | Script             |
+============+==============================+=====================+====================+
| libpupnp_  | UPnP protocol support        |                     | install-pupnp.sh   |
|            | by **pupnp** (XOR libnpupnp),|                     |                    |
|            | enabled by default           |                     |                    |
+------------+------------------------------+---------------------+--------------------+
| libnpupnp_ | UPnP protocol support        | WITH\_NPUPNP        | install-npupnp.sh  |
|            | by **npupnp** (XOR libupnp), |                     |                    |
|            | disabled by default          |                     |                    |
+------------+------------------------------+---------------------+--------------------+
| libuuid    | Depends on OS.               |                     |                    |
|            | Not required on \*BSD        |                     |                    |
+------------+------------------------------+---------------------+--------------------+
| pugixml_   | XML file and data support    |                     | install-pugixml.sh |
+------------+------------------------------+---------------------+--------------------+
| jsoncpp_   | JSON data support            |                     | install-jsoncpp.sh |
+------------+------------------------------+---------------------+--------------------+
| libiconv   | Charset conversion           |                     |                    |
+------------+------------------------------+---------------------+--------------------+
| sqlite3    | Database storage             |                     |                    |
+------------+------------------------------+---------------------+--------------------+
| zlib       | Data compression             |                     |                    |
+------------+------------------------------+---------------------+--------------------+
| fmtlib_    | Fast string formatting       |                     | install-fmt.sh     |
+------------+------------------------------+---------------------+--------------------+
| spdlog_    | Runtime logging              |                     | install-spdlog.sh  |
+------------+------------------------------+---------------------+--------------------+

Optional Packages
-----------------

+---------------------+----------------------------+-------------------------+----------+------------------------------+
| Library             | Note                       | Compile-time option     | Default  | Script                       |
+=====================+============================+=========================+==========+==============================+
| curl                | Enables web services       | WITH\_CURL              | Enabled  |                              |
+---------------------+----------------------------+-------------------------+----------+------------------------------+
| duktape_            | Scripting Support          | WITH\_JS                | Enabled  | install-duktape.sh           |
+---------------------+----------------------------+-------------------------+----------+------------------------------+
| ffmpeg/libav        | File metadata.             | WITH\_AVCODEC           | Disabled |                              |
|                     | Required for ``.m4a``.     |                         |          |                              |
+---------------------+----------------------------+-------------------------+----------+------------------------------+
| ffmpegthumbnailer_  | Generate video thumbnails  | WITH\_FFMPEGTHUMBNAILER | Disabled | install-ffmpegthumbnailer.sh |
+---------------------+----------------------------+-------------------------+----------+------------------------------+
| googletest_         | Running tests              | WITH\_TESTS             | Disabled | install-googletest.sh        |
+---------------------+----------------------------+-------------------------+----------+------------------------------+
| inotify             | Efficient file monitoring  | WITH\_INOTIFY           | Enabled  |                              |
+---------------------+----------------------------+-------------------------+----------+------------------------------+
| lastfmlib_          | Enables scrobbling         | WITH\_LASTFM            | Disabled | install-lastfm.sh            |
+---------------------+----------------------------+-------------------------+----------+------------------------------+
| libebml_            | Required for MKV           | WITH\_MATROSKA          | Enabled  | install-ebml.sh              |
+---------------------+----------------------------+-------------------------+----------+------------------------------+
| libexif_            | JPEG Exif metadata         | WITH\_EXIF              | Enabled  | install-libexif.sh           |
+---------------------+----------------------------+-------------------------+----------+------------------------------+
| libexiv2_           | Exif, IPTC, XMP metadata   | WITH\_EXIV2             | Disabled | install-libexiv2.sh          |
+---------------------+----------------------------+-------------------------+----------+------------------------------+
| libmagic            | File type detection        | WITH\_MAGIC             | Enabled  |                              |
+---------------------+----------------------------+-------------------------+----------+------------------------------+
| libmatroska_        | MKV metadata               | WITH\_MATROSKA          | Enabled  | install-matroska.sh          |
+---------------------+----------------------------+-------------------------+----------+------------------------------+
| mysql               | Alternate database storage | WITH\_MYSQL             | Disabled |                              |
+---------------------+----------------------------+-------------------------+----------+------------------------------+
| taglib_             | Audio tag support          | WITH\_TAGLIB            | Enabled  | install-taglib.sh            |
+---------------------+----------------------------+-------------------------+----------+------------------------------+
| wavpack_            | WavPack metadata support   | WITH\_WAVPACK           | Disabled | install-wavpack.sh           |
+---------------------+----------------------------+-------------------------+----------+------------------------------+

.. _doxygen: https://github.com/doxygen/doxygen
.. _duktape: https://duktape.org
.. _ffmpegthumbnailer: https://github.com/dirkvdb/ffmpegthumbnailer
.. _fmtlib: https://github.com/fmtlib/fmt
.. _googletest: https://github.com/google/googletest
.. _jsoncpp: https://github.com/open-source-parsers/jsoncpp
.. _lastfmlib: https://github.com/dirkvdb/lastfmlib
.. _libebml: https://github.com/Matroska-Org/libebml
.. _libexif: https://github.com/libexif/libexif
.. _libexiv2: https://github.com/Exiv2/exiv2
.. _libmatroska: https://github.com/Matroska-Org/libmatroska
.. _libnpupnp: https://www.lesbonscomptes.com/upmpdcli/npupnp-doc/libnpupnp.html
.. _libpupnp: https://github.com/pupnp/pupnp
.. _pugixml: https://github.com/zeux/pugixml
.. _spdlog: https://github.com/gabime/spdlog
.. _taglib: https://taglib.org/
.. _wavpack: https://www.wavpack.com/

Scripts for installation of (build) dependencies from source can be found under ``scripts``. They normally install the latest tested version
of the package. The scripts automatically install prerequisites for debian/ubuntu and opensuse/suse.

In order to build the whole package there are ``scripts/debian/build-deb.sh`` and ``scripts/opensuse/build-suse.sh`` which also take the
different requirements of distribution versions into account.

Make sure no conflicting versions of the development packages are installed.

Additional cmake Options
------------------------

+------------------------+-----------------------------------------------------+----------+
| Option                 | Note                                                | Default  |
+========================+=====================================================+==========+
| WITH\_SYSTEMD          | Install Systemd unit file                           | Enabled  |
+------------------------+-----------------------------------------------------+----------+
| WITH\_ONLINE_\SERVICES | Enable support for Online Services.                 | Disabled |
|                        | Currently there is no online service left           |          |
+------------------------+-----------------------------------------------------+----------+
| WITH\_DEBUG            | Enables debug logging                               | Enabled  |
+------------------------+-----------------------------------------------------+----------+
| WITH\_DEBUG\_OPTIONS   | Enables dedicated debug messages                    | Enabled  |
+------------------------+-----------------------------------------------------+----------+
| STATIC\_LIBUPNP        | Link to libupnp statically                          | Disabled |
+------------------------+-----------------------------------------------------+----------+
| BUILD\_DOC             | Add ``doc`` target to generate source documentation | Disabled |
|                        | Requires doxygen_ to be installed                   |          |
+------------------------+-----------------------------------------------------+----------+
| BUILD\_CHANGELOG       | Build ``changelog.gz`` file for Debian packages     | Disabled |
+------------------------+-----------------------------------------------------+----------+
| INSTALL\_DOC           | Install generated documentation into target         | Disabled |
+------------------------+-----------------------------------------------------+----------+


.. index:: Quick Start Build

Quick Start Build
~~~~~~~~~~~~~~~~~

::

  git clone https://github.com/gerbera/gerbera.git
  mkdir build
  cd build
  cmake ../gerbera -DWITH_MAGIC=1 -DWITH_MYSQL=1 -DWITH_CURL=1 -DWITH_JS=1 \
    -DWITH_TAGLIB=1 -DWITH_AVCODEC=1 -DWITH_FFMPEGTHUMBNAILER=1 -DWITH_EXIF=1 -DWITH_LASTFM=0
  make -j4
  sudo make install

If you want to have the same build as the official release:
::

  git clone https://github.com/gerbera/gerbera.git
  mkdir build
  cd build
  cmake ../gerbera --preset=release-pupnp
  make -j4
  sudo make install


Alternatively, the options can be set using a GUI (make sure to press "c" to configure after toggling settings in the GUI):
::

  git clone https://github.com/gerbera/gerbera.git
  mkdir build
  cd build
  cmake ../gerbera
  make edit_cache
  # Enable some of the WITH... options
  make -j4
  sudo make install


.. index:: Conan

Using Conan
~~~~~~~~~~~

The simplest way to fetch dependencies and build Gerbera is to use Conan.
Please read more :ref:`here <gerbera-conan>`.

.. index:: Ubuntu

Build On Ubuntu 16.04
~~~~~~~~~~~~~~~~~~~~~

::

  apt-get install uuid-dev libsqlite3-dev libmysqlclient-dev \
  libmagic-dev libexif-dev libcurl4-openssl-dev libspdlog-dev libpugixml-dev
  # If building with LibAV/FFmpeg (-DWITH_AVCODEC=1)
  apt-get install libavutil-dev libavcodec-dev libavformat-dev libavdevice-dev \
  libavfilter-dev libavresample-dev libswscale-dev libswresample-dev libpostproc-dev


The following packages are too old in 16.04 and must be installed from source:
**taglib (1.11.x)**, and **libupnp (1.8.x).**

**libupnp** must be configured/built with ``--enable-ipv6``. See
``scripts/install-pupnp.sh`` for details.

Build On Ubuntu 18.04
~~~~~~~~~~~~~~~~~~~~~

To build gerbera on Ubuntu 18.04 you have to install a newer version of the gcc++ compiler and clang++:

::

  sudo add-apt-repository -y ppa:ubuntu-toolchain-r/test
  sudo apt update
  sudo apt upgrade
  sudo apt install -y build-essential xz-utils curl gcc-8 g++-8 clang clang-9 libssl-dev  pkg-config
  sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-7 30
  sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-8 60
  sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-7 30
  sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-8 60
  sudo update-alternatives --install /usr/bin/clang clang /usr/bin/clang-9 60
  sudo update-alternatives --install /usr/bin/clang clang /usr/bin/clang-6.0 30
  sudo update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-9 60
  sudo update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-6.0 30
  sudo update-alternatives --config gcc
  sudo update-alternatives --config g++
  sudo update-alternatives --config clang
  sudo update-alternatives --config clang++
  sudo update-alternatives --set cc /usr/bin/clang
  sudo update-alternatives --set c++ /usr/bin/clang++

::

  git clone https://github.com/Kitware/CMake
  cd CMake
  ./configure
  make
  sudo make install
  cd ..


Install all libraries gerbera needs. Because they are to old libupnp, libfmt must be
build and installed from the source:

::

  sudo apt install -y uuid-dev libsqlite3-dev libmysqlclient-dev libmagic-dev \
  libexif-dev libcurl4-openssl-dev libspdlog-dev libpugixml-dev libavutil-dev \
  libavcodec-dev libavformat-dev libavdevice-dev libavfilter-dev libavresample-dev \
  libswscale-dev libswresample-dev libpostproc-dev duktape-dev libmatroska-dev \
  libsystemd-dev libtag1-dev ffmpeg


Build and install libupnp with the ``--enable-ipv6``, ``--enable-reuseaddr`` and ``--disable-blocking-tcp-connections``
options and libfmt from source

::

  wget "https://github.com/pupnp/pupnp/releases/download/release-1.14.19/libupnp-1.14.19.tar.bz2" -O libupnp-1.14.19.tar.bz2
  tar -xf libupnp-1.14.19.tar.bz2
  cd libupnp-1.14.19
  ./configure --enable-ipv6 --enable-reuseaddr --disable-blocking-tcp-connections
  make
  sudo make install
  cd ..
  git clone https://github.com/fmtlib/fmt
  cd fmt
  cmake .
  make
  sudo make install
  cd ../..


It is strongly recommended to to rebuild spdlog without bundled fmt:

::

  git clone https://github.com/gabime/spdlog
  cd spdlog
  cmake -D "SPDLOG_FMT_EXTERNAL:BOOL=true" .
  make
  sudo make install


Now it's time to get the source of gerbera and compile it.

::

  git clone https://github.com/gerbera/gerbera.git
  mkdir build
  cd build
  cmake -DWITH_MAGIC=1 -DWITH_MYSQL=1 -DWITH_CURL=1 -DWITH_JS=1 -DWITH_TAGLIB=1 -DWITH_AVCODEC=1 -DWITH_EXIF=1 -DWITH_LASTFM=0 -DWITH_SYSTEMD=1 ../gerbera
  make
  sudo make install


.. index:: Debian

Build On Debian
~~~~~~~~~~~~~~~

Necessary dependencies will be installed automatically by
``scripts/debian/build-deb.sh``.


.. index:: Debian Buster

Build On Debian Buster (10)
~~~~~~~~~~~~~~~~~~~~~~~~~~~

This guide is based on building Gerbera on Pogo Kirkwood Armel architecture boxes running Debian buster.

1. Create a swapfile when using 128Mb devices (and probably 256MB too). Add a HD or SSD but not a USB stick because of the risk of hardware failure.

2. If you for libnpupnp and libupnpp6 from https://www.lesbonscomptes.com/upmpdcli/ - Follow the build instructions to create Debian packages which you can then install with dpkg.

3. Build the latest Taglib ``cmake; make -j2`` and run ``make install`` to install or use ``scripts/install-taglib.sh``

4. Use Apt-get to install the rest of the dev packages as per dependencies list. It is best to load fmtlib-dev and libspdlog.dev from the Buster Backports

5. Clone the Gerbera git and edit the CMakeLists.txt file and comment the original version and add the new.

::

  # set(GERBERA_VERSION "git")
  set(GERBERA_VERSION "1.6.4-185-gae283931+d")

and add these lines to make the debian package
::

  SET(CPACK_GENERATOR "DEB")
  SET(CPACK_DEBIAN_PACKAGE_MAINTAINER "KK")
  # include (cmake)
  include(packaging)

6. This is the Cmake command:

::

  cmake -g DEB ../gerbera -DWITH_NPUPNP=YES -DWITH_JS=1 -DWITH_MYSQL=1 -DWITH_CURL=1 -DWITH_TAGLIB=1 -DWITH_MAGIC=1 -DWITH_MATROSKA=0 -DWITH_AVCODEC=1 -DWITH_EXIF=1 -DWITH_EXIV2=0 -DWITH_LASTFM=0 -DWITH_FFMPEGTHUMBNAILER=1 -DWITH_INOTIFY=1

Resolve any dependency issues now!

7. the ``make -j2`` will take at least some hours - go for a walk, read a book, grab some sleep .....

8. ``cpack -G DEB`` will create a debian package file - All being well - no errors. Use dpkg to install.

9. follow the gerbera manual for installation. Create the gerbera user (give the user a home directory e.g. /home/gerbera). Make the /etc/gerbera folder and get the config.xml. Symbolic link the config file:

::

  ln -s /etc/gerbera/config.xml /home/gerbera/.config/gerbera

Symbolic link the web directory:
::

  ln -s /usr/share/gerbera /usr/local/share

10. Edit ``config.xml`` and change the path to

::

  <home>/home/gerbera/.config/gerbera</home>

11. Start gerbera with the standard launch command. The server should start - watch the messages for errors. Check the web interface functions too. when happy that all is good - control-c to get back to shell

::

  gerbera -c /etc/gerbera/config.xml

12. For SystemD users, copy the gerbera.service script into /usr/systemd/system and edit it to correct the path to the gerbera server the use the systemctl command as per the manual to start and stop the server and debug any problems.

::

  ExecStart=/usr/bin/gerbera -c /etc/gerbera/config.xml

13. For init.d users, you need a gerbera script which I took from the earlier version which is in the Debian APT library

14. You need to put your new gerbera package on hold to prevent apt-get upgrade downgrading back to 1.1

::

  apt-mark hold gerbera

That should be everything you need. Gerbera version 1.6.4-185 build with this guide was running on a PogoPlug V2E02 and a V4 Pro quite happily using vlc and bubbleupnp as clients on to a fire stick and chromecast devices.


.. index:: FreeBSD

Build On FreeBSD
~~~~~~~~~~~~~~~~

`The following has been tested on FreeBSD 11.0 using a clean jail environment.`

1. Install the required :ref:`prerequisites <gerbera-requirements>` as root using either ports or packages. This can be done via Package manager or ports.
(pkg manager is used here.)  Include mysql if you wish to use that instead of SQLite3.

::

  pkg install wget git autoconf automake libtool taglib cmake gcc libav ffmpeg \
  libexif pkgconf liblastfm gmake


2. Clone repository, build depdences in current in ports and then build gerbera.

::

  git clone https://github.com/gerbera/gerbera.git
  mkdir build
  cd build
  sh ../gerbera/scripts/install-pupnp.sh
  sh ../gerbera/scripts/install-duktape.sh
  cmake ../gerbera -DWITH_MAGIC=1 -DWITH_MYSQL=0 -DWITH_CURL=1 -DWITH_JS=1 -DWITH_TAGLIB=1 -DWITH_AVCODEC=1 \
    -DWITH_EXIF=1 -DWITH_LASTFM=0 -DWITH_SYSTEMD=0
  make -j4
  sudo make install


.. index:: macOS


Build On macOS
~~~~~~~~~~~~~~

`The following has been tested on macOS High Sierra 10.13.4`

The Gerbera Team maintains a Homebrew Tap to build and install Gerbera Media Server. Take a look
at the Homebrew formula to see an example of how to compile Gerbera on macOS: `homebrew-gerbera/gerbera.rb <https://github.com/gerbera/homebrew-gerbera/blob/master/gerbera.rb>`__


.. index:: Build Docker Container On Ubuntu


Build Docker Container On Ubuntu
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Install required tools in Ubuntu
::

  sudo apt-get install docker.io git

Simplest way of building:
::

  sudo docker build https://github.com/gerbera/gerbera.git

After successfull build you should get something like
::

  Successfully built a13ccc793373

Afterwards start the container like described in the `Gerbera Docker <https://hub.docker.com/r/gerbera/gerbera>`__
documentation while replacing "gerbera/gerbera:vX.X.X" with the unique ID reported at the end of the build.

To change the compile options of Gerbera split up the process.
Download the project:
::

  git clone https://github.com/gerbera/gerbera.git

Then modify the compile parameter values in gerbera/Dockerfile. Also additional libraries might be required.
E.g. to build a container with exiv2 support add the compile option "-DWITH_EXIV2=YES" and the library
"exiv2-dev" in the first "RUN apk" command and "exiv2" in the second "RUN apk" command in the gerbera/Dockerfile.
To start the build enter
::

  sudo docker build gerbera/


.. index:: Support Development Effort

Support Development Effort
==========================

We welcome pull requests from everyone!

Gerbera was originally based on MediaTomb which is an old project that we started working on modernising - there were a lot of cobwebs.

For new code please use "modern" C++ (up to 17) constructs where possible.

First Steps
~~~~~~~~~~~

1. Fork gerbera repo https://github.com/gerbera/gerbera.

2. Clone your fork:

::

    git clone git@github.com:your-username/gerbera.git

3. Make your change.

4. Compile and run ``ctest`` if tests are enabled

5. Push to your fork

6. Submit a `pull request <https://github.com/gerbera/gerbera/compare>`__.

Some things that will increase the chance that your pull request is accepted:

- Stick to `Webkit style <https://webkit.org/code-style-guidelines/>`__.
- Format your code with ``clang-format``.
- Ensure your code works as expected by running it.
- Write a `good commit message <http://tbaggery.com/2008/04/19/a-note-about-git-commit-messages.html>`__.

It is also a good idea to run cmake with ``-DWITH_TESTS -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_CXX_FLAGS="-Werror"``
options for development.


Guidelines for Special Topics
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Some topics require proper handling, so we explain the core steps here.

* :doc:`Configuration Options </dev-configuration>`
* :doc:`Database Schema </dev-database>`
* :doc:`Conan </dev-conan>`

.. toctree::
   :maxdepth: 1
   :hidden:

   Configuration Options         <dev-configuration>
   Database Schema               <dev-database>
   Conan                         <dev-conan>
