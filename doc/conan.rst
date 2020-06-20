.. _gerbera-conan:

Developing with Conan
=====================

`Conan <https://conan.io>`_ is a C++ package manager which simplifies
setting up development environment quite a lot.

To set up everything from scratch
(assuming that generic tools like python3, C++ compiler and CMake are already installed):

Setting up Conan
----------------

From `Conan documentation <https://docs.conan.io/en/latest/installation.html>`_:

.. code-block:: bash

  # Install Conan
  $ pip3 install --user conan

  # Auto-detect system settings
  $ conan profile new default --detect
  $ conan profile update settings.compiler.cppstd=17 default
  # If using gcc:
  $ conan profile update settings.compiler.libcxx=libstdc++11 default


Building Gerbera
----------------

.. code-block:: bash

  # Get dependencies and generate build files:
  # The commands generate build system in build/ subfolder
  $ conan install -pr ./conan/gerbera-dev -if build . && conan build --configure -bf build .

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

.. note::

  Most likely you need to add ``--build missing`` option to ``conan install``.

If your system has an outdated CMake
::::::::::::::::::::::::::::::::::::

Add to ``~/.conan/profiles/default``:

.. code-block:: ini

  [build_requires]
  cmake/3.17.3

Then just clean the build directory and rerun ``conan install && conan build``.

There may be no prebuilt pacakge with particular compiler / settings
::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

Conan has prebuilt binaries for a fairly new Linux distro (and thus libc)
which may not work on an old one.

Also you may want to build dependencies with some specific flags, for example
``-flto`` to get better codegen.

You need to run rebuild missing or all packages:

.. code-block:: bash

  # Build only missing pacakges
  $ conan install --build=missing ...

  # Rebuild all packages
  $ conan install --build ...

See `Conan documentation <https://docs.conan.io/en/latest/reference/commands/consumer/install.html#build-options>`_.  

Use Conan profiles
::::::::::::::::::

It is possible to alter some options for consumed libraries
(like static / shared) or build configuration (Debug / Release)
via Conan. Conan provides a way to group such options into a profile:
a text file used in ``install`` command.

It is also possible to define custom compile / link flags in the profile.

There is a number of profiles in the ``conan`` subfolder you can use for reference.

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
