#!/bin/sh
if [ -e readme.lyx ]; then
    lyx -e docbook-xml readme.lyx
    xmlto xhtml-nochunks -m sections.xsl readme.xml
    perl ../scripts/readme_xhtml_div_extract.pl < readme.html > readme_ruby.html
else
    echo readme.lyx not found
fi

