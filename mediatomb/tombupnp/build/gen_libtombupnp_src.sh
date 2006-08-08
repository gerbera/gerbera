echo 'libtombupnp_a_SOURCES = \'

find ../ | egrep '\.(c|h)$' | sed 's/$/ \\/' | sort 

echo "dummy.h"

