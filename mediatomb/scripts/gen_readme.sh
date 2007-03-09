#!/bin/sh
if [ -e readme.lyx ]; then
    rm -f readme.xml && \
    lyx -e docbook-xml readme.lyx && \
    xmlto xhtml-nochunks -m sections.xsl readme.xml && \
    perl ../scripts/readme_xhtml_div_extract.pl < readme.html > readme_part.html && \
    xmlto txt -m sections.xsl readme.xml && \
    mv readme.txt README.UTF_8 && \
    perl ../scripts/readme_utf-8_acsii_convert.pl < README.UTF_8 > README && \
    echo finished successfully
else
    echo readme.lyx not found
fi

