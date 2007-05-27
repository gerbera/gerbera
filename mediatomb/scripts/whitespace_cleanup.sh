#!/bin/sh

find web src tombupnp config scripts/js |egrep '\.(cc?|h|html?|js|css|xsd)$' | xargs perl scripts/test.pl
