.. index:: Install Packages

Installing Gerbera
==================

We strongly advise using the packages provided by your distribution where provided.
Please see below for guides on how to install Gerbera on various distributions.


Ubuntu/Mint
~~~~~~~~~~~~~~~~~
.. index:: Ubuntu Linux
.. index:: Mint

Stephen Czetty maintains a `Ubuntu PPA <https://launchpad.net/~stephenczetty/+archive/ubuntu/gerbera-updates>`_.

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

CentOS
~~~~~~~~~~~~~~~~~
Gerbera 1.2 for centos x86/64 is available via GitHub https://github.com/lukesoft76/CENTOS_7.
Gerbera 1.2 only installs correctly if some dependencies are satisfied:
Duktape
libupnp
taglib
All neccessary rpm files are listed in the provided github project https://github.com/lukesoft76/CENTOS_7 .

Attention! Sofar, Gerbera is not part of any repository that is mantained in CentOS 7 due to the fact that Gerbera is only 
availalbe for Fedora 28 which is not the base for CentOS 7!

Debian
~~~~~~~~~~~~~~~~~
.. index:: Debian Linux

Gerbera is included in `Testing <https://packages.debian.org/buster/gerbera>`_ and `Unstable <https://packages.debian.org/sid/gerbera>`_.

.. code-block:: sh

    sudo apt install gerbera

openSUSE
~~~~~~~~~~~~~~~~~
.. index:: openSUSE Linux

Gerbera is available on `software.opensuse.org <https://software.opensuse.org/package/gerbera>`_.



Entware (Optware)
~~~~~~~~~~~~~~~~~
.. index:: Entware
.. index:: Optware

Gerbera is available in `Entware <https://github.com/Entware/rtndev/tree/master/gerbera>`_ for your embedded device/router!


macOS
~~~~~
.. index:: macOS

Gerbera is available as the `Gerbera Homebrew Tap <https://github.com/gerbera/homebrew-gerbera/>`_ on macOS.

