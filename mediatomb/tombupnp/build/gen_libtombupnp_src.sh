echo 'libtombupnp_a_SOURCES = \'

find ../src | egrep '\.(c|h)$' | sed 's/$/ \\/' | sort 

echo "dummy.h"

