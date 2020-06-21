.. _gerbera-conan:

Developing with Conan
=====================

`Conan <https://conan.io>`_ is a C++ package manager which simplifies
setting up development environment quite a lot.

To set up everything from scratch
(assuming that generic tools like python3, C++ compiler and CMake are already installed):

Setting up Conan
----------------

From `Conan installation instructions <https://docs.conan.io/en/latest/installation.html>`_:

.. code-block:: bash

  # Install Conan
  $ pip3 install --user conan

  # Auto-detect system settings
  $ conan profile new default --detect
  # If using gcc:
  $ conan profile update settings.compiler.libcxx=libstdc++11 default


Building Gerbera
----------------

.. code-block:: bash

  # Get dependencies (building them if needed) and generate build files:
  # The commands generate build system in build/ subfolder
  $ conan install -pr ./conan/dev --build missing -if build . && conan build --configure -bf build .

  # Now project is ready to build.

.. note::

  Conan also installs a number of packages using system package manager.
  Therefore it's a good idea to make sure that ``sudo`` works without password.
  Check also documentation for CONAN_SYSREQUIRES_MODE_.

.. _CONAN_SYSREQUIRES_MODE: https://docs.conan.io/en/latest/reference/env_vars.html#env-vars-conan-sysrequires-mode

.. note::
  
  Although it is possible to run CMake directly to configure the project
  (and Conan reference shows this multiple times in an example),
  it's much more convenient to invoke it via conan helper.

  This way customizations needs to be defined only once on the Conans
  command line (via ``-o`` parameter), which will affect dependencies
  (e.g. ``testing`` or ``js``) and turned into ``WITH_*`` switches to CMake.

Tweaking
--------

Use Ninja
:::::::::

.. code-block:: bash

  $ conan config set general.cmake_generator=Ninja  


Set up different compiler
:::::::::::::::::::::::::

Add to ``~/.conan/profiles/default``:

.. code-block:: ini

  [settings]
  compiler.version=10

  [env]
  CC=gcc-10
  CXX=g++-10

If your system has an outdated CMake
::::::::::::::::::::::::::::::::::::

Add to ``~/.conan/profiles/default``:

.. code-block:: ini

  [build_requires]
  cmake/3.17.3

Then just clean the build directory and rerun ``conan install && conan build``.

There may be no prebuilt pacakge with particular compiler / settings
::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

Conan has prebuilt binaries, but they may not be suitable.
Some may be built with older C++ standard
(`conan-center-index#1984 <https://github.com/conan-io/conan-center-index/issues/1984>`_),
while others may require newer libc
(`conan-docker-tools#205 <https://github.com/conan-io/conan-docker-tools/issues/205>`_).

Therefore the most reasonable default is to build missing binary packages
(which is handled nicely by Conan).


Also you may want to build dependencies with some specific flags, for example
``-flto`` to get better codegen.

You need to run rebuild missing or all packages:

.. code-block:: bash

  # Build only missing packages
  $ conan install --build=missing ...

  # Rebuild all packages
  $ conan install --build=force ...

See `Conan documentation <https://docs.conan.io/en/latest/reference/commands/consumer/install.html#build-options>`_.  

Use Conan profiles
::::::::::::::::::

It is possible to alter some options for consumed libraries
(like static / shared) or build configuration (Debug / Release)
via Conan. Conan provides a way to group such options into a profile:
a text file used in ``install`` command.

It is also possible to define custom compile / link flags in the profile.

There is a number of profiles in the ``conan`` subfolder you can use for reference.

Cleanup
:::::::

Conan stores all data in ``$HOME/.conan`` just remove this folder to free disk space.

To remove only packages use ``conan remove -f '*'``

Searching for a package (or checking an update)
:::::::::::::::::::::::::::::::::::::::::::::::

.. code-block:: bash

  $ conan search "fmt" -r all
  
  Existing package recipes:

  Remote 'conan-center':
  ...
  fmt/6.1.2
  fmt/6.2.0
  fmt/6.2.1

Building on FreeBSD
:::::::::::::::::::

Everything works almost out of the box, except that there are no prebuilt packages.

.. code-block:: bash

  # Python for Conan
  $ pkg install python3 py37-pip py37-sqlite3

  # Tools to build dependencies
  $ pkg install autoconf automake libtool pkgconf gmake

  # Fix build for Iconv
  $ conan config set general.conan_make_program=gmake

Remaining system packages are managed by Conan.

.. warning::

  ``conan_make_program`` is needed to build correctly IConv. However it interferes with
  CMake generator (if set to Ninja), so please switch to Ninja after building all dependencies.

.. warning::

  It is not a good idea to build with GCC on FreeBSD since resulting binaries crash
  because system uses CLang and its libc++ which is incompatible with gccs libstdc++.

Cross-building
::::::::::::::

This is an example for Raspberry Pi 3 on Ubuntu host.

.. code-block:: bash

  $ apt install g++-10-aarch64-linux-gnu
  $ conan profile new raspberry-pi3

Populate file with content:

.. code-block:: ini

  toolchain=/usr/aarch64-linux-gnu
  target_host=aarch64-linux-gnu
  cc_compiler=gcc-10
  cxx_compiler=g++-10

  [env]
  CONAN_CMAKE_FIND_ROOT_PATH=$toolchain
  CHOST=$target_host
  AR=$target_host-ar
  AS=$target_host-as
  RANLIB=$target_host-ranlib
  CC=$target_host-$cc_compiler
  CXX=$target_host-$cxx_compiler
  STRIP=$target_host-strip

  [settings]
  os=Linux
  arch=armv8
  compiler=gcc

  compiler.version=10
  compiler.libcxx=libstdc++11

.. code-block:: bash

  $ conan install -pr:b default -pr:h ./conan/release -pr:h ./conan/minimal -pr:h raspberry-pi3 --build missing -if build . && conan build --configure -bf build .
  $ cd build && make
  ...
  [100%] Linking CXX executable gerbera
  [100%] Built target gerbera
  build $ file gerbera 
  gerbera: ELF 64-bit LSB shared object, ARM aarch64, version 1 (SYSV), dynamically linked, interpreter /lib/ld-linux-aarch64.so.1, BuildID[sha1]=7bfdb98dd51a1a5dda5101a0e9f090806fb35a41, for GNU/Linux 3.7.0, with debug_info, not stripped
  $  aarch64-linux-gnu-strip -s -o gerbera-s gerbera 
  $ du -hs gerbera-s 
  3.9M    gerbera-s

This is a minimal example to begin with.
If you have packages from the target system you may omit the minimal profile
or tune options on the command line.
