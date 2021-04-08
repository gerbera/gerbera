.. index:: Install Packages

Installing Gerbera
==================

We strongly advise using the packages provided by your distribution if available.
Please see below for guides on how to install Gerbera on various distributions.

Docker
~~~~~~~~~~~~~~~~~
.. index:: Docker

Docker images are provided on `Docker Hub <https://hub.docker.com/r/gerbera/gerbera>`_, we recommend running a tagged version.

.. code-block:: sh

    docker pull gerbera/gerbera

Ubuntu/Mint
~~~~~~~~~~~~~~~~~
.. index:: Ubuntu Linux
.. index:: Mint

We maintain a `Ubuntu Repository <https://gerbera.jfrog.io/>`_.

To install the latest tagged release (>=1.8.0):

.. code-block:: sh

    curl -fsSL https://gerbera.jfrog.io/artifactory/api/gpg/key/public | sudo apt-key add -
    sudo apt-add-repository https://gerbera.jfrog.io/artifactory/debian

Or for the latest code install git builds:

.. code-block:: sh

    curl -fsSL https://gerbera.jfrog.io/artifactory/api/gpg/key/public | sudo apt-key add -
    sudo apt-add-repository https://gerbera.jfrog.io/artifactory/debian-git

Or Stephen Czetty maintains a `Ubuntu PPA <https://launchpad.net/~stephenczetty/+archive/ubuntu/gerbera-updates>`_.

.. code-block:: sh

    sudo add-apt-repository ppa:stephenczetty/gerbera-updates
    sudo apt-get update
    sudo apt install gerbera

Gentoo
~~~~~~~~~~~~~~~~~
.. index:: Gentoo

The latest version and live ebuild are in `the main portage tree <https://packages.gentoo.org/packages/net-misc/gerbera>`_.

.. code-block:: sh

    emerge -va net-misc/gerbera

Arch
~~~~~~~~~~~~~~~~~
.. index:: Arch Linux

Gerbera is available in AUR with both `stable <https://aur.archlinux.org/packages/gerbera/>`_ or `git versions <https://aur.archlinux.org/packages/gerbera-git/>`_.

Fedora
~~~~~~~~~~~~~~~~~
.. index:: Fedora

Gerbera is available in Fedora 29 or later.

.. code-block:: sh

    sudo dnf install gerbera

FreeBSD
~~~~~~~~~~~~~~~~~
.. index:: FreeBSD

Gerbera is available via packages and ports collection.

.. code-block:: sh

    pkg install gerbera

or

.. code-block:: sh

   cd /usr/ports/net/gerbera/ && make install clean

CentOS
~~~~~~~~~~~~~~~~~
.. index:: CentOS

Gerbera 1.2 for Centos x86/64 is available via GitHub: https://github.com/lukesoft76/CENTOS_7.

All necessary rpm files are listed in the provided github project https://github.com/lukesoft76/CENTOS_7 .

Attention! So far, Gerbera is not part of any repository that is maintained in CentOS 7 due to the fact that Gerbera is only
available for Fedora 28 which is not the base for CentOS 7!

Debian
~~~~~~~~~~~~~~~~~
.. index:: Debian Linux

Gerbera is included in `Buster <https://packages.debian.org/buster/gerbera>`_ and `Sid <https://packages.debian.org/sid/gerbera>`_.

.. code-block:: sh

    sudo apt install gerbera

Due to the stable nature of Debian, these packages are likely to be some versions behind the current Gerbera release.

`Deb-Multimedia.org <https://www.deb-multimedia.org/>`_ also provide builds for `Buster <http://www.deb-multimedia.org/dists/buster/main/binary-amd64/package/gerbera>`_ and `Sid <http://www.deb-multimedia.org/dists/sid/main/binary-amd64/package/gerbera>`_.

openSUSE
~~~~~~~~~~~~~~~~~
.. index:: openSUSE Linux

Gerbera is available on `software.opensuse.org <https://software.opensuse.org/package/gerbera>`_.

Entware (Optware)
~~~~~~~~~~~~~~~~~
.. index:: Entware
.. index:: Optware

Gerbera is available in `Entware <https://github.com/Entware/rtndev/tree/master/gerbera>`_ for your embedded device/router!


OpenWrt (OpenWrt)
~~~~~~~~~~~~~~~~~
.. index:: OpenWrt

Gerbera is available in `OpenWrt <https://github.com/openwrt/packages/tree/master/multimedia/gerbera>`_ for your embedded device/router!


macOS
~~~~~
.. index:: macOS

Gerbera is available as the `Gerbera Homebrew Tap <https://github.com/gerbera/homebrew-gerbera/>`_ on macOS.

