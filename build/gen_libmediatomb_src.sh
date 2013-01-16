echo 'libmediatomb_a_SOURCES = \'

find ../src | egrep '\.(cc?|h)$' | sed 's/$/ \\/' | sort 

echo "dummy.h"

