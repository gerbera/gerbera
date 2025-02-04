.. index:: Install Packages

Installing Gerbera
==================

We strongly advise using the packages provided by your distribution if available.
Please see below for guides on how to install Gerbera on various distributions.

Please try to follow the n-1 principle, i.e. do not install versions older that the previous release.
Be aware that this page may not fully reflect latest changes in the respective distribution as well as
Team Gerbera is not responsible for the way the packages are built by others.
When installing older versions it is likely that options described in this documentation may not exist.

Docker
~~~~~~~~~~~~~~~~~
.. index:: Docker

Docker images are provided on `Docker Hub <https://hub.docker.com/r/gerbera/gerbera>`__, we recommend running a tagged version.

On Raspbian the container must be run with ``privileged: true`` to get time sync running.

.. code-block:: sh

    docker pull gerbera/gerbera

Don't forget to provide your configuration and volume to the image.

Ubuntu/Mint
~~~~~~~~~~~~~~~~~
.. index:: Ubuntu Linux
.. index:: Mint

We maintain a `Ubuntu Repository <https://pkg.gerbera.io/>`__.

To install the latest tagged release (>=2.1.0):

.. code-block:: sh

    wget -O - https://pkg.gerbera.io/public.asc 2> /dev/null | gpg --dearmor - | sudo tee /usr/share/keyrings/gerbera-keyring.gpg > /dev/null
    echo "deb [signed-by=/usr/share/keyrings/gerbera-keyring.gpg] https://pkg.gerbera.io/debian/ $(lsb_release -c --short) main" | sudo tee /etc/apt/sources.list.d/gerbera.list > /dev/null
    sudo apt-get update
    sudo apt-get install gerbera

Or for the latest code install git builds:

.. code-block:: sh

    wget -O - https://pkg.gerbera.io/public.asc 2> /dev/null | gpg --dearmor - | sudo tee /usr/share/keyrings/gerbera-keyring.gpg > /dev/null
    echo "deb [signed-by=/usr/share/keyrings/gerbera-keyring.gpg] https://pkg.gerbera.io/debian-git/ $(lsb_release -c --short) main" | sudo tee /etc/apt/sources.list.d/gerbera.list > /dev/null
    sudo apt-get update
    sudo apt-get install gerbera

If you want to use the package for Mint you have to set the codename manually, instead of ``$(lsb_release -c --short)``


Gentoo
~~~~~~~~~~~~~~~~~
.. index:: Gentoo

The latest version and live ebuild are in `the main portage tree <https://packages.gentoo.org/packages/net-misc/gerbera>`__.

.. code-block:: sh

    emerge -va net-misc/gerbera

Arch
~~~~~~~~~~~~~~~~~
.. index:: Arch Linux

Gerbera is `packaged for Arch <https://archlinux.org/packages/?name=gerbera>`__.

.. code-block:: sh

    pacman -S gerbera

AUR has `git versions <https://aur.archlinux.org/packages/gerbera-git/>`__.

Fedora
~~~~~~~~~~~~~~~~~
.. index:: Fedora

Gerbera is available in Fedora 29 or later.

.. code-block:: sh

    sudo dnf install gerbera

If you are running the Server edition of Fedora you will probably need to configure your firewall to
open the ports Gerbera uses. In the default firewall configuration the following commands should do the 
trick (change ``49152`` to the port Gerbera is actually using, see the
:ref:`Port <troubleshoot_port>` section of the Troubleshooting page).
If you are running the Workstation edition of Fedora, Gerbera should work with the default 
firewall configuration and these commands won't be needed.

.. code-block:: sh

    sudo firewall-cmd --permanent --add-service=ssdp
    sudo firewall-cmd --permanent --add-port=49152/tcp

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
~~~~~~
.. index:: Debian Linux

Gerbera is included in Buster_ and Sid_.

.. code-block:: sh

    sudo apt install gerbera

Due to the stable nature of Debian, these packages are likely to be some versions behind the current Gerbera release.

We maintain a `Debian Repository <https://pkg.gerbera.io/>`__.

To install the latest tagged release (>=2.1.0):

.. code-block:: sh

    wget -O - https://pkg.gerbera.io/public.asc 2>/dev/null | gpg --dearmor - | sudo tee /usr/share/keyrings/gerbera-keyring.gpg >/dev/null
    echo "deb [signed-by=/usr/share/keyrings/gerbera-keyring.gpg] https://pkg.gerbera.io/debian/ $(lsb_release -c --short) main" | sudo tee /etc/apt/sources.list.d/gerbera.list >/dev/null
    sudo apt-get update
    sudo apt-get install gerbera

Or for the latest code install git builds:

.. code-block:: sh

    wget -O - https://pkg.gerbera.io/public.asc 2>/dev/null | gpg --dearmor - | sudo tee /usr/share/keyrings/gerbera-keyring.gpg >/dev/null
    echo "deb [signed-by=/usr/share/keyrings/gerbera-keyring.gpg] https://pkg.gerbera.io/debian-git/ $(lsb_release -c --short) main" | sudo tee /etc/apt/sources.list.d/gerbera.list >/dev/null
    sudo apt-get update
    sudo apt-get install gerbera


`Deb-Multimedia.org <https://www.deb-multimedia.org/>`__ also provide builds for Buster_ and Sid_.

.. _Buster: http://www.deb-multimedia.org/dists/buster/main/binary-amd64/package/gerbera
.. _Sid: http://www.deb-multimedia.org/dists/sid/main/binary-amd64/package/gerbera

openSUSE
~~~~~~~~
.. index:: openSUSE Linux

Gerbera is available on `software.opensuse.org <https://software.opensuse.org/package/gerbera>`__.

OpenWrt (OpenWrt)
~~~~~~~~~~~~~~~~~
.. index:: OpenWrt

Gerbera is available in `OpenWrt <https://github.com/openwrt/packages/tree/master/multimedia/gerbera>`__ for your embedded device/router!


Buildroot
~~~~~~~~~~~~~~~~~
.. index:: Buildroot

Gerbera is available in `Buildroot <https://git.buildroot.net/buildroot/tree/package/gerbera>`__.


macOS
~~~~~
.. index:: macOS

Gerbera is available as the `Gerbera Homebrew Tap <https://github.com/gerbera/homebrew-gerbera/>`__ on macOS.

