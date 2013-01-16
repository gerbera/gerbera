echo 'libtombupnp_a_SOURCES = \'

find ../ | egrep '\.(c|h)$' | egrep -v 'win_dll\.c' | sed 's/$/ \\/' | sort 

echo "dummy.h"

