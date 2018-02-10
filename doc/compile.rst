.. index:: Compile Gerbera

Compile Gerbera
===============

The project has been ported to `CMake <https://cmake.org/>`_.


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


.. index:: Ubuntu

Build On Ubuntu 16.04
~~~~~~~~~~~~~~~~~~~~~

::

  apt-get install uuid-dev libexpat1-dev libsqlite3-dev libmysqlclient-dev \
  libmagic-dev libexif-dev libcurl4-openssl-dev
  # If building with LibAV/FFmpeg (-DWITH_AVCODEC=1)
  apt-get install libavutil-dev libavcodec-dev libavformat-dev libavdevice-dev \
  libavfilter-dev libavresample-dev libswscale-dev libswresample-dev libpostproc-dev


The following packages are too old in 16.04 and must be installed from source:
**taglib (1.11.x)**, and **libupnp (1.8.x).**

**libupnp** must be configured/built with ``--enable-ipv6``. See
``scripts/install-pupnp18.sh`` for details.

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

  git clone https://github.com/v00d00/gerbera.git
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

`The following has been tested on macOS Sierra 10.12.6`

1. Install :ref:`prerequisites <gerbera-requirements>` using homebrew
2. Install **ossp-uuid** using homebrew
3. Install **pupnp** and **duktape** using the scripts provided.
4. Use the cmake option ``-DCMAKE_FIND_FRAMEWORK=LAST`` to avoid Clang linkage errors
