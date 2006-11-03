#!/bin/sh

find web src |egrep '\.(cc?|h|html?|js)$' | xargs perl scripts/update-comment-header.pl
