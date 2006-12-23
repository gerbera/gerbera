#!/bin/sh

find web src tombupnp |egrep '\.(cc?|h|html?|js)$' | xargs perl scripts/update-comment-header.pl
perl scripts/update-comment-header.pl configure.ac
