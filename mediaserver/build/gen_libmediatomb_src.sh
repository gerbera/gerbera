echo 'libmediatomb_a_SOURCES = \'

find ../src -name '*.c' | sed 's/\.c/.c \\/'
find ../src -name '*.cc' | sed 's/\.cc/.cc \\/'
find ../src -name '*.h' | sed 's/\.h/.h \\/'

echo dummy.h

