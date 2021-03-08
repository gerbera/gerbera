.. index:: Compile Gerbera

Compile Gerbera
===============

Gerbera uses the excellent `CMake <https://cmake.org/>`_ build system.


.. _gerbera-requirements:

Dependencies
~~~~~~~~~~~~

.. Note:: Remember to install associated development packages, because development headers are needed for compilation!

In order to compile Gerbera you will have to install the following packages:

+---------------------+---------------+-----------------+------------------------------+---------------------------+------------+
| Library             | Min Version   | Required?       | Note                         | Compile-time option       | Default    |
+=====================+===============+=================+==============================+===========================+============+
| libupnp             | 1.12.1        | XOR libnpupnp   | [pupnp]                      |                           |            |
+---------------------+---------------+-----------------+------------------------------+---------------------------+------------+
| libnpupnp           | 4.0.11        | XOR libupnp     | [npupnp]                     | WITH\_NPUPNP              | Disabled   |
+---------------------+---------------+-----------------+------------------------------+---------------------------+------------+
| libuuid             |               | Depends on OS   | Not required on \*BSD        |                           |            |
+---------------------+---------------+-----------------+------------------------------+---------------------------+------------+
| pugixml             |               | Required        | [pugixml]                    |                           |            |
+---------------------+---------------+-----------------+------------------------------+---------------------------+------------+
| libiconv            |               | Required        |                              |                           |            |
+---------------------+---------------+-----------------+------------------------------+---------------------------+------------+
| sqlite3             | 3.7.0         | Required        | Database storage             |                           |            |
+---------------------+---------------+-----------------+------------------------------+---------------------------+------------+
| zlib                |               | Required        |                              |                           |            |
+---------------------+---------------+-----------------+------------------------------+---------------------------+------------+
| fmtlib              | 5.3           | Required        |                              |                           |            |
+---------------------+---------------+-----------------+------------------------------+---------------------------+------------+
| spdlog              |               | Required        |                              |                           |            |
+---------------------+---------------+-----------------+------------------------------+---------------------------+------------+
| duktape             | 2.1.0         | Optional        | Scripting Support            | WITH\_JS                  | Enabled    |
+---------------------+---------------+-----------------+------------------------------+---------------------------+------------+
| mysql               |               | Optional        | Alternate database storage   | WITH\_MYSQL               | Disabled   |
+---------------------+---------------+-----------------+------------------------------+---------------------------+------------+
| curl                |               | Optional        | Enables web services         | WITH\_CURL                | Enabled    |
+---------------------+---------------+-----------------+------------------------------+---------------------------+------------+
| [taglib]            | 1.11.1        | Optional        | Audio tag support            | WITH\_TAGLIB              | Enabled    |
+---------------------+---------------+-----------------+------------------------------+---------------------------+------------+
| libmagic            |               | Optional        | File type detection          | WITH\_MAGIC               | Enabled    |
+---------------------+---------------+-----------------+------------------------------+---------------------------+------------+
| libmatroska         |               | Optional        | MKV metadata                 | WITH\_MATROSKA            | Enabled    |
+---------------------+---------------+-----------------+------------------------------+---------------------------+------------+
| ffmpeg/libav        |               | Optional        | File metadata                | WITH\_AVCODEC             | Disabled   |
+---------------------+---------------+-----------------+------------------------------+---------------------------+------------+
| libexif             |               | Optional        | JPEG Exif metadata           | WITH\_EXIF                | Enabled    |
+---------------------+---------------+-----------------+------------------------------+---------------------------+------------+
| libexiv2            |               | Optional        | Exif, IPTC, XMP metadata     | WITH\_EXIV2               | Disabled   |
+---------------------+---------------+-----------------+------------------------------+---------------------------+------------+
| lastfmlib           | 0.4.0         | Optional        | Enables scrobbling           | WITH\_LASTFM              | Disabled   |
+---------------------+---------------+-----------------+------------------------------+---------------------------+------------+
| ffmpegthumbnailer   |               | Optional        | Generate video thumbnails    | WITH\_FFMPEGTHUMBNAILER   | Disabled   |
+---------------------+---------------+-----------------+------------------------------+---------------------------+------------+
| inotify             |               | Optional        | Efficient file monitoring    | WITH\_INOTIFY             | Enabled    |
+---------------------+---------------+-----------------+------------------------------+---------------------------+------------+


.. index:: Quick Start Build

Quick Start Build
~~~~~~~~~~~~~~~~~

::

  git clone https://github.com/gerbera/gerbera.git
  mkdir build
  cd build
  cmake ../gerbera -DWITH_MAGIC=1 -DWITH_MYSQL=1 -DWITH_CURL=1 -DWITH_JS=1 \
  -DWITH_TAGLIB=1 -DWITH_AVCODEC=1 -DWITH_FFMPEGTHUMBNAILER=1 -DWITH_EXIF=1 -DWITH_LASTFM=1
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
``scripts/install-pupnp18.sh`` for details.

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


Build and install libupnp with the ``--enable-ipv6`` and ``--enable-reuseaddr`` option and libfmt from sourcec

::

  wget "https://downloads.sourceforge.net/project/pupnp/pupnp/libupnp-1.12.1/libupnp-1.12.1.tar.bz2?r=https%3A%2F%2Fsourceforge.net%2Fprojects%2Fpupnp%2Ffiles%2Flatest%2Fdownload&ts=1588248015" -O libupnp-1.12.1.tar.bz2
  tar -xf libupnp-1.12.1.tar.bz2
  cd libupnp-1.12.1
  ./configure --enable-ipv6 --enable-reuseaddr
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


.. index:: Debian Buster

Build On Debian Buster
~~~~~~~~~~~~~~~~~~~~~~

This guide is based on buildinh Gerbera on Pogo Kirkwood Armel architecture boxes running Debian buster.

1. Create a swapfile when using 128Mb devices (and probably 256MB too). Add a HD or SSD but not a USB stick because of the risk of hardware failure.

2. If you for libnpupnp and libupnpp6 from https://www.lesbonscomptes.com/upmpdcli/ - Follow the build instructions to create Debian packages which you can then install with dpkg.

3. Build the latest Taglib [`cmake; make -j2`] and use `make install` to install

4. Use Apt-get to install the rest of the dev packages as per dependencies list. It is best to load fmtlib-dev and libspdlog.dev from the Buster Backports

5. Clone the Gerbera git and edit the CMakeLists.txt file and comment the original version and add the new.

`# set(GERBERA_VERSION "git")`
`set(GERBERA_VERSION "1.6.4-185-gae283931+d")`

and add these lines to make the debian package

`SET(CPACK_GENERATOR "DEB")`
`SET(CPACK_DEBIAN_PACKAGE_MAINTAINER "KK")`
`# include (cmake)`
`include(packaging)`

6. This is the Cmake command:

`cmake -g DEB ../gerbera -DWITH_NPUPNP=YES -DWITH_JS=1 -DWITH_MYSQL=1 -DWITH_CURL=1 -DWITH_TAGLIB=1 -DWITH_MAGIC=1 -DWITH_MATROSKA=0 -DWITH_AVCODEC=1 -DWITH_EXIF=1 -DWITH_EXIV2=0 -DWITH_LASTFM=0 -DWITH_FFMPEGTHUMBNAILER=1 -DWITH_INOTIFY=1`

Resolve any dependency issues now!

7. the `make -j2` will take at least some hours - go for a walk, read a book, grab some sleep .....

8. `cpack -G DEB` will create a debian package file - All being well - no errors. Use dpkg to install.

9. follow the gerbera manual for installation. Create the gerbera user (give the user a home directory e.g. /home/gerbera). Make the /etc/gerbera folder and get the config.xml. Symbolic link the config file:

`ln -s /etc/gerbera/config.xml /home/gerbera/.config/gerbera`

Symbolic link the web directory:

`ln -s /usr/share/gerbera /usr/local/share`

10. Edit `config.xml` and change the path to

`<home>/home/gerbera/.config/gerbera</home>`

11. Start gerbera with the standard launch command. The server should start - watch the messages for errors. Check the web interface functions too. when happy that all is good - control-c to get back to shell

`gerbera -c /etc/gerbera/config.xml`

12. For SystemD users, copy the gerbera.service script into /usr/systemd/system and edit it to correct the path to the gerbera server the use the systemctl command as per the manual to start and stop the server and debug any problems.

`ExecStart=/usr/bin/gerbera -c /etc/gerbera/config.xml`

13. For init.d users, you need a gerbera script which I took from the earlier version which is in the Debian APT library

14. You need to put your new gerbera package on hold to prevent apt-get upgrade downgrading back to 1.1

`apt-mark hold gerbera`

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
  sh ../gerbera/scripts/install-pupnp18.sh
  sh ../gerbera/scripts/install-duktape.sh
  cmake ../gerbera -DWITH_MAGIC=1 -DWITH_MYSQL=0 -DWITH_CURL=1 -DWITH_JS=1 -DWITH_TAGLIB=1 -DWITH_AVCODEC=1 \
  -DWITH_EXIF=1 -DWITH_LASTFM=0 -DWITH_SYSTEMD=0
  make -j4
  sudo make install


.. index:: macOS

Build On macOS
~~~~~~~~~~~~~~

`The following has been tested on macOS High Sierra 10.13.4`

The Gerbera Team maintains a Homebrew Tap to build and install Gerbera Media Server.  Take a look
at the Homebrew formula to see an example of how to compile Gerbera on macOS.

`homebrew-gerbera/gerbera.rb <https://github.com/gerbera/homebrew-gerbera/blob/master/gerbera.rb>`_
