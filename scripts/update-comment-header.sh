#!/bin/sh

find web webnew src tombupnp config scripts/js | grep -E '\.(cc?|h|html?|js|css|xsd)$' | xargs perl scripts/update-comment-header.pl
perl scripts/update-comment-header.pl configure.ac
