echo 'libmediatomb_a_SOURCES = \'

find .. -name '*.c' | sed 's/\.c/.c \\/'
find .. -name '*.cc' | sed 's/\.cc/.cc \\/'
find .. -name '*.h' | sed 's/\.h/.h \\/'

echo dummy.h

