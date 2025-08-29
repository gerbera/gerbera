.. index:: Configuration Overview

#############
Configuration
#############

Gerbera is highly configurable and allows the user to set various options and preferences that define the server's
behavior. Rather than enforcing certain features upon the user, we prefer to offer a number of choices where possible.
The heart of Gerbera configuration is the config.xml file, which is located in the ``~/.config/gerbera`` directory.
If the configuration file is not found in the default location and no configuration was specified on the command line,
Gerbera will generate a default config.xml file in the ``~/.config/gerbera`` directory. The file is in the XML format and can
be edited by a simple text editor, here is the list of all available options:

Below are links to the configuration sections

* :doc:`Generate Config File </config-generate>`
* :doc:`General </config-general>`
* :doc:`Server </config-server>`
* :doc:`Extended Runtime Options </config-extended>`
* :doc:`Client Options </config-clients>`
* :doc:`Import Content </config-import>`
* :doc:`Online Content </config-online>`
* :doc:`Transcode Content </config-transcode>`

.. toctree::
   :maxdepth: 1
   :hidden:

   Config Generation        <config-generate>
   Configuration Basics     <config-general>
   Server Options           <config-server>
   Extended Options         <config-extended>
   Content Import Options   <config-import>
   Online Content Options   <config-online>
   Client Options           <config-clients>
   Transcoding Options      <config-transcode>
