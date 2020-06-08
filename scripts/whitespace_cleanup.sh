#!/bin/sh

find web src tombupnp config scripts/js | grep -E '\.(cc?|h|html?|js|css|xsd)$' | xargs perl scripts/whitespace_cleanup.pl
