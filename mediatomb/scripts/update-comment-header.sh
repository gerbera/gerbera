#!/bin/sh

find web src tombupnp |egrep '\.(cc?|h|html?|js|css)$' | xargs perl scripts/update-comment-header.pl
perl scripts/update-comment-header.pl configure.ac
