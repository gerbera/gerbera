.. _gerbera-conan:

Developing with Conan
=====================

`Conan <https://conan.io>`_ is a C++ package manager which simplifies
setting up development environment quite a lot.

To set up everything from scratch
(assuming that generic tools like python3, C++ compiler and CMake are already installed):

Setting up Conan
----------------

.. code-block:: bash

  # Install Conan
  $ pip3 install --user conan

  # Auto-detect system settings
  $ conan profile new default --detect
  $ conan profile update settings.compiler.libcxx=libstdc++11 default
  $ conan profile update settings.compiler.cppstd=17 default


Building Gerbera
----------------

.. code-block:: bash

  # Build directory of out of source build
  $ mkdir build && cd build

  # Get dependencies and generate build files:
  $ conan install .. && conan build --configure ..

  # Now project is ready to build.

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
Set environment variable ``CONAN_CMAKE_GENERATOR`` to ``Ninja``.


Set up different compiler, add to your Conan profile:
:::::::::::::::::::::::::::::::::::::::::::::::::::::

.. code-block:: ini

  [settings]
  compiler.version=10

  [env]
  CC=gcc-10
  CXX=g++-10

There may be no prebuilt pacakge with particular compiler / settings
::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

.. code-block:: bash

  $ conan install --build=missing ...

A profile for development:
::::::::::::::::::::::::::

.. code-block:: bash

  $ conan profile new gerbera

Example content:

.. code-block:: ini

  include(default)

  [settings]
  build_type=Debug
  [options]
  *:shared=True
  gerbera:debug_logging=True
  gerbera:tests=True
  
  [env]
  CXXFLAGS=-Og -Werror -Wall
  # Requires CMake >= 3.17
  CMAKE_EXPORT_COMPILE_COMMANDS=ON


Then use in ``conan install`` command:

.. code-block:: bash

  $ conan install -pr gerbera ...

(Or just add this content to default profile)

A stand-alone binary with all static dependenceis
:::::::::::::::::::::::::::::::::::::::::::::::::

.. code-block:: bash

  $ conan install -o "*:shared=False" -o "*:fPIC=False" ...

(Or put this into a profile, together with ``CXXFLAGS=-O3 -flto``)

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
