#!/bin/sh

gen_readme () {

if [ -e $FILENAME.lyx ]; then
    rm -f $FILENAME.xml && \
    lyx -e docbook-xml $FILENAME.lyx && \
    xmlto xhtml-nochunks -m sections.xsl $FILENAME.xml && \
    perl ../scripts/readme_xhtml_div_extract.pl < ${FILENAME}.html > ${FILENAME}_part.html && \
    xmlto txt -m sections.xsl $FILENAME.xml && \
    mv $FILENAME.txt $FILENAME_UTF_8 && \
    perl ../scripts/readme_utf-8_acsii_convert.pl < $FILENAME_UTF_8 > $FILENAME_ASCII && \
    echo finished successfully
else
    echo $FILENAME.lyx not found
fi

}

FILENAME=readme
FILENAME_ASCII=README
FILENAME_UTF_8=$FILENAME_ASCII.UTF_8
gen_readme

FILENAME=scripting
FILENAME_ASCII=scripting.txt
FILENAME_UTF_8=scripting_utf8.txt
gen_readme

FILENAME=ui
FILENAME_ASCII=ui.txt
FILENAME_UTF_8=ui_utf8.txt
gen_readme
